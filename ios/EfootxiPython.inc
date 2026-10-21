// CPython 3.12 embedded bridge. Runtime/assets are assembled locally from pinned inputs.
#ifdef EFOOTXI_IOS
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <CommonCrypto/CommonCryptor.h>

static PyObject* nativeAES(PyObject*, PyObject* args) {
  int encrypt; const char *key, *iv, *input; Py_ssize_t keySize, ivSize, size;
  if (!PyArg_ParseTuple(args, "iy#y#y#", &encrypt, &key, &keySize, &iv, &ivSize, &input, &size)) return nullptr;
  if (keySize != 32 || ivSize != 16 || size % 16) { PyErr_SetString(PyExc_ValueError, "AES-256-CBC requires key=32, iv=16, block-aligned input"); return nullptr; }
  PyObject* output = PyBytes_FromStringAndSize(nullptr, size+16);
  if (!output) return nullptr;
  size_t written = 0;
  CCCryptorStatus status = CCCrypt(encrypt ? kCCEncrypt : kCCDecrypt, kCCAlgorithmAES, 0, key, keySize, iv,
      input, size, PyBytes_AS_STRING(output), size+16, &written);
  if (status != kCCSuccess) { Py_DECREF(output); PyErr_Format(PyExc_RuntimeError, "CommonCrypto AES error %d", status); return nullptr; }
  if (_PyBytes_Resize(&output, written) < 0) return nullptr;
  return output;
}
static PyMethodDef efootxiMethods[] = {{"aes_cbc", nativeAES, METH_VARARGS, nullptr}, {nullptr,nullptr,0,nullptr}};
static struct PyModuleDef efootxiModule = {PyModuleDef_HEAD_INIT, "_efootxi_native", nullptr, -1, efootxiMethods};
PyMODINIT_FUNC PyInit__efootxi_native() { return PyModule_Create(&efootxiModule); }

static std::mutex gPythonMu;
static PyObject* gPythonServe;
static bool gPythonAttempted = false;
static NSString* gPythonFailure;

static void initializeEfootxi() {
  gPythonAttempted = true;
  NSString* bundle = NSBundle.mainBundle.bundlePath;
  NSString* python = [bundle stringByAppendingPathComponent:@"python"];
  NSString* resources = [bundle stringByAppendingPathComponent:@"efootxi"];
  NSString* support = [[gSupport stringByDeletingLastPathComponent] stringByAppendingPathComponent:@"Efootxi2027"];
  [NSFileManager.defaultManager createDirectoryAtPath:support withIntermediateDirectories:YES attributes:nil error:nil];
  if (PyImport_AppendInittab("_efootxi_native", PyInit__efootxi_native) < 0) { gPythonFailure=@"Cannot register native Python module"; return; }
  PyConfig config; PyConfig_InitIsolatedConfig(&config);
  config.write_bytecode=0; config.install_signal_handlers=0; config.parse_argv=0; config.use_environment=0;
  config.buffered_stdio=0;
  PyStatus status = PyConfig_SetBytesString(&config, &config.home, python.fileSystemRepresentation);
  if (!PyStatus_Exception(status)) status = PyConfig_SetBytesString(&config, &config.executable, NSBundle.mainBundle.executablePath.fileSystemRepresentation);
  for (NSString* path in @[[python stringByAppendingPathComponent:@"lib/python3.12"],
      [python stringByAppendingPathComponent:@"lib/python3.12/lib-dynload"], [resources stringByAppendingPathComponent:@"bridge"]]) {
    if (PyStatus_Exception(status)) break;
    wchar_t* wide = Py_DecodeLocale(path.fileSystemRepresentation, nullptr);
    if (!wide) { gPythonFailure=@"Invalid Python module path"; PyConfig_Clear(&config); return; }
    status = PyWideStringList_Append(&config.module_search_paths, wide); PyMem_RawFree(wide);
  }
  config.module_search_paths_set=1;
  if (!PyStatus_Exception(status)) status = Py_InitializeFromConfig(&config);
  if (PyStatus_Exception(status)) gPythonFailure = [NSString stringWithUTF8String:status.err_msg ?: "Python initialization failed"];
  PyConfig_Clear(&config);
  if (!Py_IsInitialized()) return;
  PyObject* module = PyImport_ImportModule("efootxi_ios");
  PyObject* result = module ? PyObject_CallMethod(module, "configure", "sss", resources.fileSystemRepresentation,
      support.fileSystemRepresentation, gSessionDir.fileSystemRepresentation) : nullptr;
  if (result) {
    Py_DECREF(result); gPythonServe = PyObject_GetAttrString(module, "serve");
  }
  if (!gPythonServe) { PyErr_Print(); gPythonFailure=@"Efootxi startup failed; see stdout-stderr.log"; }
  Py_XDECREF(module);
  PyEval_SaveThread();
  ZLog(@"[EFOOTXI] Python startup %@", gPythonServe ? @"ready" : gPythonFailure);
}

static NSData* efootxiServe(NSString* method, NSString* path, NSData* body, NSInteger* status, NSString** mime) {
  std::lock_guard<std::mutex> lock(gPythonMu);
  if (!gPythonAttempted) initializeEfootxi();
  if (!gPythonServe) { *status=500; *mime=@"text/plain"; return [(gPythonFailure ?: @"Python unavailable") dataUsingEncoding:NSUTF8StringEncoding]; }
  PyGILState_STATE gil = PyGILState_Ensure();
  PyObject* result = PyObject_CallFunction(gPythonServe, "ssy#", method.UTF8String, path.UTF8String, body.bytes ?: "", (Py_ssize_t)body.length);
  NSData* data = nil;
  int code=500; const char *type=nullptr, *bytes=nullptr; Py_ssize_t length=0;
  if (result && PyArg_ParseTuple(result, "isy#", &code, &type, &bytes, &length)) {
    *status=code; *mime=[NSString stringWithUTF8String:type]; data=[NSData dataWithBytes:bytes length:length];
  } else {
    PyErr_Print(); *status=500; *mime=@"text/plain"; data=[@"Efootxi request failed; see session logs" dataUsingEncoding:NSUTF8StringEncoding];
    ZLog(@"[EFOOTXI_ERROR] %@", path);
  }
  Py_XDECREF(result); PyGILState_Release(gil); return data;
}
#endif

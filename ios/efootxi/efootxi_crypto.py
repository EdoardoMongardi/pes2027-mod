"""AES protocol adapter; iOS uses Apple's CommonCrypto, host tests use cryptography."""
try:
    from _efootxi_native import aes_cbc
except ImportError:
    from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes

    def aes_cbc(encrypt, key, iv, data):
        cipher = Cipher(algorithms.AES(key), modes.CBC(iv))
        ctx = cipher.encryptor() if encrypt else cipher.decryptor()
        return ctx.update(data) + ctx.finalize()

#!/usr/bin/env python3
import os
import sys

KEY = 0x5A

def xor_bytes(s: str) -> list[int]:
    return [ord(c) ^ KEY for c in s]

def main():
    client_id = os.environ.get("YDISQUETTE_CLIENT_ID", "")
    client_secret = os.environ.get("YDISQUETTE_CLIENT_SECRET", "")
    out_path = sys.argv[1] if len(sys.argv) > 1 else None
    if not out_path:
        sys.exit(1)
    id_arr = xor_bytes(client_id)
    secret_arr = xor_bytes(client_secret)
    with open(out_path, "w") as f:
        f.write("#pragma once\n\n")
        f.write("#define YDISQUETTE_DEFAULT_REDIRECT_URI \"ydisquette://callback\"\n\n")
        f.write("static const unsigned char kObfuscatedId[] = {" + ",".join(str(b) for b in id_arr) + "};\n")
        f.write("static const int kObfuscatedIdLen = {};\n".format(len(id_arr)))
        f.write("static const unsigned char kObfuscatedSecret[] = {" + ",".join(str(b) for b in secret_arr) + "};\n")
        f.write("static const int kObfuscatedSecretLen = {};\n".format(len(secret_arr)))
        f.write("static const unsigned char kObfuscationKey = {};\n".format(KEY))

if __name__ == "__main__":
    main()

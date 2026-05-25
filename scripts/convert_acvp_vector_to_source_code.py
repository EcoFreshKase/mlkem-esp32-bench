hex_per_line = 8

test_vector =                 {
          "tcId": 1,
          "z": "0A64FDD51A8D91B3166C4958A94EFC3166A4F5DF680980B878DB8371B7624C96",
          "d": "BBA3C0F5DF044CDF4D9CAA53CA15FDE26F34EB3541555CFC54CA9C31B964D0C8"
        }

s = test_vector['d']
all_hex = [f"0x{s[i:i+2].upper()}" for i in range(0, len(s), 2)]
splited_hex = [all_hex[i:i+hex_per_line] for i in range(0, len(all_hex), hex_per_line)]

for hex_line in splited_hex:
    print(", ".join(hex_line), end="")
    print(",")
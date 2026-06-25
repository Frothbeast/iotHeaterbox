import math


def calculate_lut(fixed_res, is_pull_up, beta, r0=100000, t0=298.15):
    lut = []
    for adc in range(1024):
        if adc <= 5: adc = 5
        if adc >= 1020: adc = 1020

        # Calculate Resistance based on topology
        if is_pull_up:
            res = fixed_res * ((1023.0 - adc) / adc)
        else:
            res = fixed_res * (adc / (1023.0 - adc))

        # Beta Equation: 1/T = 1/T0 + 1/B * ln(R/R0)
        inv_t = (1.0 / t0) + (1.0 / beta) * math.log(res / r0)
        temp_c = (1.0 / inv_t) - 273.15

        # Store as int (temp * 10)
        lut.append(int(round(temp_c * 10)))
    return lut


# Heater (Prusa Semitec 104GT-2): Beta=4267, 698 Ohm Pull-down
h_lut = calculate_lut(698.0, True, 4267)
# Box: Beta=4350 (from your code), 50300 Ohm Pull-up
b_lut = calculate_lut(50300.0, False, 4350)


# Print as C arrays
def print_c_array(name, data):
    print(f"const int16_t {name}[1024] = {{")
    for i in range(0, 1024, 16):
        print("    " + ", ".join(map(str, data[i:i + 16])) + ",")
    print("};")


print_c_array("heater_temp_lut", h_lut)
print_c_array("box_temp_lut", b_lut)
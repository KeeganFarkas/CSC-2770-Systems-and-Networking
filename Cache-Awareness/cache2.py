import subprocess

file_name = input("Enter the name of your file: ")

for i in range(1, 9):
    avg = 0.0

    for j in range(1, 21):
        output_bytes = subprocess.check_output([f"./{file_name}", f"{i}"])
        output_str = output_bytes.decode()
        avg += float(output_str[6:-1])

    avg /= 20
    print(f"{i} {avg:.5f}")
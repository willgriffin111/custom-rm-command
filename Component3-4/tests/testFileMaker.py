import os

# Get the current working directory
current_working_directory = os.getcwd()

# Get the directory of the script
script_directory = os.path.dirname(os.path.abspath(__file__))

# Change the working directory to the script directory
os.chdir(script_directory)

# Your file creation code
for i in range(3):
    with open(f"file{i}.txt", "w") as file:
        file.write("test file")
     
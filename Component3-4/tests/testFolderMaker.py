import os

# Get the directory of the current Python script
script_dir = os.path.dirname(os.path.realpath(__file__))

# Create the path to the new folder
new_folder_path = os.path.join(script_dir, 'testFolder')

# Check if the directory exists, if not, create it
if not os.path.exists(new_folder_path):
    os.makedirs(new_folder_path)

for i in range(0,3):
    with open(os.path.join(new_folder_path, f"file{i}.txt"), "w") as file:
        file.write("test file")

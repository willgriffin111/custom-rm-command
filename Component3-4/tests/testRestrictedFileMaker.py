import os

# Function to create a file with no access in the script's directory
def create_no_access_file(filename):
    # Get the directory of the current script
    script_dir = os.path.dirname(os.path.realpath(__file__))

    # Full path for the new file
    file_path = os.path.join(script_dir, filename)

    # Create the file and write a message
    with open(file_path, 'w') as file:
        file.write("This is a test file with no access permissions.")

    # Remove all access permissions
    os.chmod(file_path, 0o000)


# File name for the new file
filename = 'resterictedFile.txt'

# Create the file
create_no_access_file(filename)
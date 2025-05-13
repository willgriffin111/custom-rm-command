# Custom `rm` Command

An implementation of the Unix `rm` command, written in C for educational purposes.

##  How to Compile

```bash
gcc src/rm4.c -o rm
```


## How to Run

Run the custom `rm` executable with the file(s) or directory path you want to remove. Examples:

```bash
./rm src/tests/file0.txt             // single file
./rm -v src/tests/file0.txt          // single file with verbose
./rm -v -i src/tests/file1.txt       // single file with verbose and interactive
./rm -r src/tests/testFolder/        // directories
./rm -r -f src/tests/testFolder/     // directories with force
```

## Test files/directories helper

Use the helper Python scripts to generate test files and folders:

```bash
python3 src/tests/testFileMaker.py
python3 src/tests/testFolderMaker.py
python3 src/tests/testEmptyFolderMaker.py
python3 src/tests/testRestrictedFileMaker.py
```
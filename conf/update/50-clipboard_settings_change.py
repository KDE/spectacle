import fileinput


def check_if_true(line):
    return line.split("=")[1].strip() == "true"


for line in fileinput.input():
    if line.startswith("copyImageToClipboard"):
        if check_if_true(line):
            print("clipboardGroup=PostScreenshotCopyImage")
        print("# DELETE copyImageToClipboard")

    elif line.startswith("copySaveLocation"):
        if check_if_true(line):
            print("clipboardGroup=PostScreenshotCopyLocation")
        print("# DELETE copySaveLocation")

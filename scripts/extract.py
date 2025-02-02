#!/usr/bin/env python3
'''
Filename: extract.py

Extracts available file list for the English dataset from the MultiUN corpus
and combines all extracted documents into a single text file.

Usage:
  $ python extract.py

For a detailed description of this corpus, please read:

  MultiUN: A Multilingual corpus from United Nation Documents,
  Andreas Eisele and Yu Chen, LREC 2010.

Please cite the paper, if you use this corpus in your work.

'''
import os, re, sys
from xml.sax import saxutils

OUTPUT_DIR = "../../../../Downloads/un/text"
XMLROOT = "./xml"
COMBINED_FILE = "./text/combined_text.txt"  # Output file for combined text

LANG_CODE = {
  'Chinese': "zh",
  'Arabic': "ar",
  'French': "fr",
  'Spanish': "es",
  'Russian': "ru",
  'English': "en",
  'Other': "de",
  'German': "de"
}

LANG_NAME = {
  "zh": 'Chinese',
  "ar": 'Arabic',
  "fr": 'French',
  "es": 'Spanish',
  "ru": 'Russian',
  "en": 'English',
  "de": 'Other'
}

HELP_MESSAGE = '''Usage:\n\tpython extract.py\n\n''' \
  '''This script extracts only the English dataset from the MultiUN corpus''' \
  ''' and combines all documents into a single text file.'''


def warning(message):
    """Prints a warning message to stderr."""
    print(message, file=sys.stderr)
    sys.exit()


STAG = re.compile(r"^[ \t]*<s n=\"\d+\">[ \t]*(.*)[ \t]*</s>[ \t]*$")
TAG = re.compile(r"<[^>]*>")

def extract_text(filename, lang="en"):
    """Extracts text from the given XML file."""
    with open(filename, 'r', encoding='utf-8') as handle:
        text = ""
        for line in handle:
            line = line.strip()

            if line.find("<s") >= 0:
                sentence = STAG.search(line).groups()[0] + "\n"

                if not len(sentence.strip()):
                    continue

                tagged = re.findall(r"<[^>]*>[^<]*</[^>]*>", sentence)
                tag_dict = {}

                if len(tagged):
                    for pos in range(len(tagged)):
                        line = tagged[pos]
                        tag_type = line.lstrip("<").split(">")[0].upper()
                        tag_type += " - " + str(pos)
                        tag_value = line.lstrip("<").split(">")[1].split("<")[0]
                        tag_dict[tag_type] = tag_value
                        sentence = sentence.replace(line, tag_type, 1)

                tok_sent = sentence.strip()

                if len(tagged):
                    for t, l in tag_dict.items():
                        tok_sent = tok_sent.replace(t, l)

                text += tok_sent.strip() + "\n"

            if line.find("</p") >= 0:
                text += "\n"
    return saxutils.unescape(text.strip(), {'&quot;': '"', "&apos;": "'"})


def combine_text_files(output_dir, combined_file):
    """Combines all .snt files in the output directory into a single file."""
    with open(combined_file, 'w', encoding='utf-8') as outfile:
        for root, _, files in os.walk(output_dir):
            for file in files:
                if file.endswith(".snt"):
                    file_path = os.path.join(root, file)
                    with open(file_path, 'r', encoding='utf-8') as infile:
                        outfile.write(infile.read())
                        outfile.write("\n")  # Add a newline between documents


if __name__ == "__main__":
    # Set the language to English
    LANGS = ["en"]

    # Prepare output directory
    TEXTDIR = os.path.join(OUTPUT_DIR, LANGS[0])
    if not os.path.exists(OUTPUT_DIR):
        os.mkdir(OUTPUT_DIR)
    if not os.path.exists(TEXTDIR):
        os.mkdir(TEXTDIR)

    COUNT = 0

    # Iterate over years (2000–2099)
    for year in range(2000, 2100):
        year_dir = os.path.join(XMLROOT, LANGS[0], str(year))

        if not os.path.exists(year_dir):
            continue

        target_dir = os.path.join(TEXTDIR, str(year))
        if not os.path.exists(target_dir):
            os.mkdir(target_dir)

        # Process each file in the year directory
        for filename in os.listdir(year_dir):
            if not filename.endswith(".xml"):
                continue

            docid = filename.rsplit('-', 1)[0]
            orgfile = os.path.join(year_dir, filename)
            newfile = os.path.join(target_dir, f"{docid}_{LANGS[0]}.snt")

            # Extract text and write to the output file
            with open(newfile, 'w', encoding='utf-8') as nf:
                nf.write(extract_text(orgfile, LANGS[0]))

            COUNT += 1

    print(f"{COUNT} English documents processed.", file=sys.stderr)

    # Combine all .snt files into a single file
    combine_text_files(TEXTDIR, COMBINED_FILE)
    print(f"All documents combined into {COMBINED_FILE}.", file=sys.stderr)
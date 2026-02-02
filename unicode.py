import os, sys, codecs
try:
  from urllib.request import Request, urlopen
except:
  from urllib2 import Request, urlopen

UNICODE_DATA_URL = "https://www.unicode.org/Public/UNIDATA/UnicodeData.txt"
UNICODE_DATA_FILE = "UnicodeData.txt"
OUTPUT_HEADER = "unicode"

# https://inventwithpython.com/blog/downloading-web-pages-without-requests.html
def download_unicode_data(force=False):
  if not os.path.exists(UNICODE_DATA_FILE) or force:
    print("Downloading %s ..." % UNICODE_DATA_FILE)
    try:
      r = Request(UNICODE_DATA_URL, headers={
        'User-agent' : 'Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:97.0) Gecko/20100101 Firefox/97.0'
      })
      req = urlopen(r)
      with codecs.open(UNICODE_DATA_FILE, 'wb') as fp:
        fp.write(req.read())
      print('Saved as %s' % UNICODE_DATA_FILE)
    except Exception as e:
      print("Excemtion: ", e)
  else:
    print('%s already exists, skipping download.' % UNICODE_DATA_FILE)

def parse_unicode_data():
  mappings = []
  with open(UNICODE_DATA_FILE, 'r') as fp:
    for line in fp:
      parts = line.strip().split(";")
      if len(parts) < 14:
        continue

      cp = int(parts[0], 16)
      lower = parts[13]
      if lower:
        lc = int(lower, 16)
        mappings.append((cp, lc))
  return sorted(mappings)

def merge_ranges(mappings):
  merged = []
  i = 0
  while i < len(mappings):
    start, lc = mappings[i]
    delta = lc - start
    end = start
    j = i + 1
    while j < len(mappings):
      next_cp, next_lc = mappings[j]
      if next_cp == end + 1 and next_lc - next_cp == delta:
        end = next_cp
        j += 1
      else:
        break
    merged.append((start, end, delta))
    i = j
  return merged

def write_header(ranges):
  cnt = 1
  text = None
  with codecs.open('%s.c' % OUTPUT_HEADER, "rb", encoding="utf-8") as fp:
    text = fp.read()

  with codecs.open('%s.c' % OUTPUT_HEADER, "w", encoding="utf-8") as fp:
    found = 0
    for line in text.splitlines():
      if 'utf8_lower_ranges[]' in line:
        fp.write("%s\n" % line)
        for s, e, d in ranges:
          if cnt % 2 == 0:
            fp.write(" { 0x%04x, 0x%04x, %d },\n" % (s, e, d))
          else:
            fp.write("  { 0x%04x, 0x%04x, %d }," % (s, e, d))
          cnt += 1
        fp.write("  { 0, 0, 0 }\n")
        found = 1

      if found == 1:
        if '};' not in line:
          continue
        fp.write("%s\n" % line)
        found = 0
        continue

      fp.write("%s\n" % line)
    

if __name__ == "__main__":
  if len(sys.argv) > 1:
    download_unicode_data(True)
  else:
    download_unicode_data()
  mappings = parse_unicode_data()
  ranges = merge_ranges(mappings)
  write_header(ranges)
  print("Done! Total ranges: %d" % len(ranges))

# text parts
processed_files = { }

# authors
for filename in ['AUTHORS', 'CONTRIBUTORS']:
  with open(filename) as f:
    text = ''
    for line in f:
      if filename == 'AUTHORS':
        text += '// fast_float by ' + line
      if filename == 'CONTRIBUTORS':
        text += '// with contributions from ' + line
    processed_files[filename] = text

# licenses
for filename in ['LICENSE-MIT', 'LICENSE-APACHE']:
  with open(filename) as f:
    text = ''
    for line in f:
      text += '// ' + line
    processed_files[filename] = text

# code
for filename in [ 'fast_float.h', 'float_common.h', 'ascii_number.h', 
                  'fast_table.h', 'decimal_to_binary.h', 'ascii_number.h', 
                  'simple_decimal_conversion.h', 'parse_number.h']:
  with open('include/fast_float/' + filename) as f:
    text = ''
    for line in f:
      if line.startswith('#include "'): continue
      text += line
    processed_files[filename] = text

# command line
import argparse

parser = argparse.ArgumentParser(description='Amalgamate fast_float.')
parser.add_argument('--license', default='MIT', help='choose license')
parser.add_argument('--output', default='', help='output file (stdout if none')

args = parser.parse_args()

text = '\n\n'.join([
  processed_files['AUTHORS'], processed_files['CONTRIBUTORS'], 
  processed_files['LICENSE-' + args.license],
  processed_files['fast_float.h'], processed_files['float_common.h'], 
  processed_files['ascii_number.h'], processed_files['fast_table.h'],
  processed_files['decimal_to_binary.h'], processed_files['ascii_number.h'],
  processed_files['simple_decimal_conversion.h'],
  processed_files['parse_number.h']])

if args.output:
  with open(args.output, 'wt') as f:
    f.write(text)
else:
  print(text)

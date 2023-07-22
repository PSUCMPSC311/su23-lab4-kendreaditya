#!/bin/bash

# Create the output directory if it doesn't exist
mkdir -p my-traces

# Define the input files
input_files=("simple-input" "linear-input" "random-input")

# Define the expected output files
expected_output_files=("simple-expected-output" "linear-expected-output" "random-expected-output")

# Loop through the input files
for ((i=0; i<${#input_files[@]}; i++)); do
  input_file="${input_files[i]}"
  expected_output_file="${expected_output_files[i]}"
  output_file="my-traces/${expected_output_file/expected-output/my-output}"
  output_file_1024="my-traces/1024-${expected_output_file/expected-output/my-output}"

  # Run the test and redirect the output to a file
  ./tester -w "traces/$input_file" > "$output_file"
  ./tester -w "traces/$input_file" -s 1024 > "$output_file_1024"

  # Compare the output with the expected output
  if cmp -s "$output_file" "traces/$expected_output_file"; then
    echo "Output files $output_file and $expected_output_file are the same"
  else
    echo "Output files $output_file and $expected_output_file are different"
  fi
done

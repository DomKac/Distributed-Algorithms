#!/usr/bin/env python3
import sys 

current_month = None
temp_sum = 0
temp_count = 0
first_line = True

# read the entire line from STDIN 
for line in sys.stdin: 
	# remove leading and trailing whitespace 
	line = line.strip() 
	# splitting the data on the basis of tab we have provided in mapper.py 
	month, temp = line.split('\t', 1) 
	# convert count (currently a string) to int 
	try: 
		month = int(month)
		temp = float(temp)
	except ValueError: 
		# count was not a number, so silently 
		# ignore/discard this line 
		continue


	if first_line:
		current_month = month
		first_line = False

	# this IF-switch only works because Hadoop sorts map output 
	# by key (here: word) before it is passed to the reducer 
	if current_month == month: 
		temp_sum += temp 
		temp_count += 1
	else:
		# write result to STDOUT
		print(f"{current_month}\t{temp_sum / temp_count}")
		current_month = month
		temp_sum = temp
		temp_count = 1

# do not forget to output the last word if needed! 
if current_month == month: 
	print ('%s\t%s' % (month, temp_sum / temp_count)) 

    



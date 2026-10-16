#include "report_buffer_lab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void trim_newline(char *line) 
{
	size_t length;
	length = strlen(line);
	if (length > 0 && line[length - 1] == '\n') 
	{
		line[length - 1] = '\0';
	}
}

int load_orders(FILE *in, struct order_record records[], size_t capacity, struct lab_stats *stats) {
	char line[LAB_MAX_LINE_LEN];
	size_t count;

	if (in == NULL || records == NULL || stats == NULL) { return -1; }

	memset(stats, 0, sizeof(*stats));
	count = 0;

	while (fgets(line, sizeof(line), in) != NULL) {
		// Width limited scan
		char name[LAB_MAX_NAME_LEN];
		char category[LAB_MAX_CATEGORY_LEN];
		int quantity;
		int unit_price;
		
		stats->input_reads++;
		trim_newline(line);

		if (line[0] == '\0') { continue; }

		if (count == capacity) 
		{
			fprintf(stderr, "too many records\n");
			return -1;
		}

		/* TODO(student):
		   - parse line with a width-limited sscanf() pattern
		   - reject malformed lines
		   - compute total_price
		   - update longest_name, grand_total, and max_total
		*/

		if (sscanf(line, "%31[^|]|%d|%d|%15[^|]", name, &quantity, &unit_price, category) != 4) 
		// If fewer than 4 field is filled (!=4) return -1
		{ return -1; }

		// Store the parsed information into records
		snprintf(records[count].name, sizeof(records[count].name), "%s", name);
		snprintf(records[count].category, sizeof(records[count].category), "%s", category);
		records[count].quantity = quantity;
		records[count].unit_price = unit_price;

		// Calculate total price
		records[count].total_price = quantity * unit_price;

		/*
		snprintf(records[count].name, sizeof(records[count].name), "%s", line);
		snprintf(records[count].category, sizeof(records[count].category), "unknown");
		records[count].quantity = 0;
		records[count].unit_price = 0;
		records[count].total_price = 0;
		count++;
		*/

		// Update longest name
		if (strlen(name) > stats->longest_name) {stats->longest_name = strlen(name);}

		// Update grand total
		stats->grand_total += records[count].total_price;

		// Update max total
		if (records[count].total_price > stats->max_total)
		{ stats->max_total = records[count].total_price; }

		count++;
	}

	stats->records_loaded = count;
	return 0;
}

int build_report(const struct order_record records[], size_t count, const struct lab_stats *stats,
		 char *out, size_t out_size) 
{
	int written;
	if (records == NULL || stats == NULL || out == NULL || out_size == 0) { return -1; }
	size_t offset;
	size_t i;

	/* TODO(student):
	   - use snprintf() to append into out
	   - print the header line first
	   - print each row using stats->longest_name for flexible alignment
	   - print the summary line last
	   - fail if the report buffer is too small
	*/

	offset = 0;

	written = snprintf(out + offset, out_size - offset, "report | rows=%zu | longest=%zu\n", count, stats->longest_name);

	// written = snprintf(out, out_size, "TODO(student): implement build_report for %zu records\n", count);
	
	if (written < 0 || (size_t)written >= out_size - offset) 
	{ return -1; }

	offset += (size_t)written;


	for (i = 0; i < count; i++) 
	{
		written = snprintf(out + offset,
			out_size - offset,
			"%02zu | %-*s | qty=%2d | unit=%3d | total=%3d | cat=%s\n",
			i + 1,
			(int)stats->longest_name,
			records[i].name,
			records[i].quantity,
			records[i].unit_price,
			records[i].total_price,
			records[i].category
		);

		if (written < 0 || (size_t)written >= out_size - offset) { return -1; }

		offset += (size_t)written;
	}

	written = snprintf(out + offset,
		out_size - offset,
		"summary | grand_total=%d | max_total=%d | reads=%zu | writes=%zu\n",
		stats->grand_total,
		stats->max_total,
		stats->input_reads,
		stats->output_writes
	);

	if (written < 0 || (size_t)written >= out_size - offset) { return -1 ;}

	return 0;
}

int main(int argc, char **argv) 
{
	FILE *in;
	struct order_record records[LAB_MAX_RECORDS];
	struct lab_stats stats;
	char report[LAB_REPORT_CAPACITY];
	size_t report_length;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <orders-file>\n", argv[0]);
		return 1;
	}

	in = fopen(argv[1], "r");
	if (in == NULL) {
		perror("fopen");
		return 1;
	}

	if (load_orders(in, records, LAB_MAX_RECORDS, &stats) != 0) {
		fclose(in);
		return 1;
	}

	if (fclose(in) != 0) {
		perror("fclose");
		return 1;
	}

	stats.output_writes = 1;

	if (build_report(records, stats.records_loaded, &stats, report, sizeof(report)) != 0) {
		fprintf(stderr, "failed to build report\n");
		return 1;
	}

	report_length = strlen(report);
	if (fwrite(report, 1, report_length, stdout) != report_length) {
		perror("fwrite");
		return 1;
	}

	return 0;
}
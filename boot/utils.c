#include "utils.h"

bool strcmp(const char *s1, const char *s2)
{
	unsigned int i = 0;

	// While we didn't reach the end of one of the strings
	while (*(s1 + i) && *(s2 + i)) {
		// If the current char in each string differ
		if (*(s1 + i) != *(s2 + i)) {
			return false;
		}

		i++;
	}

	return *s1 == *s2;
}
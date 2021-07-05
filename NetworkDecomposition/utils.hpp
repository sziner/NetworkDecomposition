#pragma once
unsigned int log2(unsigned int v)
{
	unsigned int r = 0;
	while (v >>= 1) r++;
	return r;
}
#pragma once
#include <fstream>
#include <vector>
#include "../build/Gateware.h"



std::vector<GW::MATH::GMATRIXF> MatrixList;

void parseGameLvl(const char* FileName) 
{
	std::ifstream openFile; // Fstream var to open file
	openFile.open("../GameLevel.txt", std::ios::in | std::ios::out);
	GW::MATH::GMATRIXF InstanceM;
	if (!openFile.fail()) {
		while (true) {
			if (openFile.eof()) { break; } // If end of file, stop loop
			

		}
	}
}


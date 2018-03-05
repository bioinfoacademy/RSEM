/* Copyright (c) 2017
   Bo Li (The Broad Institute of MIT and Harvard)
   libo@broadinstitute.org

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 3 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.   

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA
*/

#include <new>
#include <cmath>
#include <cstring>
#include <cassert>
#include <string>
#include <fstream>

#include "utils.h"
#include "NoiseQProfile.hpp"

NoiseQProfile::NoiseQProfile(int mode) : mode(mode), p(NULL), c(NULL), ss(NULL) {
	p = new double[QSIZE][NCODES];
	if (mode == 0) {
		c = new double[QSIZE][NCODES];
		ss = new double[QSIZE][NCODES];
	}
}

NoiseQProfile::~NoiseQProfile() {
	if (p != NULL) delete[] p;
	if (c != NULL) delete[] c;
	if (ss != NULL) delete[] ss;
}

void NoiseQProfile::clear() {
	memset(p, 0, sizeof(double) * QSIZE * NCODES);
}

void NoiseQProfile::collect(const NoiseQProfile* o) {
	for (int i = 0; i < QSIZE; ++i)
		for (int j = 0; j < NCODES; ++j)
			p[i][j] += o->p[i][j];
}

void NoiseQProfile::finish() {
	calc_ss() // calculate sufficient statistics	
	ss2p(); // calculate p
	p2logp(); // convert to log space
}

double NoiseQProfile::calcLogP() const {
	double logp = 0.0;
	
	for (int i = 0; i < QSIZE; ++i)
		for (int j = 0; j < NCODES; ++j)
			if (c[i][j] > 0.0) logp += c[i][j] * p[i][j]; // p in log space

	return logp;
}

void NoiseQProfile::read(std::ifstream& fin, int choice) {
	std::string line;
	int tmp_qsize, tmp_ncodes;
	double (*in)[NCODES] = NULL;

	switch(choice) {
		case 0: in = p; break;
		case 1: in = ss; break;
		case 2: in = c;
	}

	assert((fin>> tmp_qsize>> tmp_ncodes) && (tmp_qsize = QSIZE) && (tmp_ncodes == NCODES));
	for (int i = 0; i < QSIZE; ++i)
		for (int j = 0; j < NCODES; ++j)
			assert(fin>> in[i][j]);
	getline(fin, line);

	if (mode == 0 && choice == 0) p2logp();
	if (mode == 2) prepare_for_simulation();
}

void NoiseQProfile::write(std::ofstream& fout, int choice) {
	double (*out)[NCODES] = NULL;

	switch(choice) {
		case 0: ss2p(); out = p; break;
		case 1: out = ss; break;
		case 2: out = c;
	}

	fout<< "#nqpro\tNoiseQProfile\t"<< 3 + QSIZE<< "\tformat: QSIZE NCODES; p[QSIZE][NCODES], QSIZE lines with each line containing NCODES values"<< std::endl;
	fout<< QSIZE<< '\t'<< NCODES<< std::endl;
	for (int i = 0; i < QSIZE; ++i) {
		for (int j = 0; j < NCODES - 1; ++j) fout<< out[i][j]<< '\t';
		fout<< out[i][j]<< std::endl;
	}
	fout<< std::endl<< std::endl;
}

void NoiseQProfile::calc_ss() {
	for (int i = 0; i < QSIZE; ++i)
		for (int j = 0; j < NCODES; ++j) 
			ss[i][j] = p[i][j] + c[i][j];
}

void NoiseQProfile::ss2p() {
	double sum;
	for (int i = 0; i < QSIZE; ++i) {
		sum = 0.0;
		for (int j = 0; j < NCODES; ++j) {
			p[i][j] = ss[i][j] + pseudoC[j];
			sum += p[i][j];
		}
		for (int j = 0; j < NCODES; ++j)
			p[i][j] /= sum;
	}
}

void NoiseQProfile::p2logp() {
	for (int i = 0; i < QSIZE; ++i)
		for (int j = 0; j < NCODES; ++j)
			p[i][j] = log(p[i][j]);
}

void NoiseQProfile::prepare_for_simulation() {
	for (int i = 0; i < QSIZE; ++i) 
		for (int j = 1; j < NCODES; ++j)
			p[i][j] += p[i][j - 1];
}

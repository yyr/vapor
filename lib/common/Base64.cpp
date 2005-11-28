
#include <string>
#include <cassert>
#include <vapor/Base64.h>

using namespace VetsUtil;

Base64::Base64() {

	int	i;
	// set up encoding and decoding tables
	//
	for(i= 0;i<9;i++){
		_eTable[i]= 'A'+i;
		_eTable[i+9]= 'J'+i;
		_eTable[26+i]= 'a'+i;
		_eTable[26+i+9]= 'j'+i;
	}
	for(i= 0;i<8;i++){
		_eTable[i+18]= 'S'+i;
		_eTable[26+i+18]= 's'+i;
	}
	for(i= 0;i<10;i++){
		_eTable[52+i]= '0'+i;
	}
	_eTable[62]= '+';
	_eTable[63]= '/';


	for(i= 0;i<255;i++) _dTable[i]= 0x80;

	for(i= 'A';i<='I';i++) _dTable[i]= 0+(i-'A');

	for(i= 'J';i<='R';i++) _dTable[i]= 9+(i-'J');

	for(i= 'S';i<='Z';i++) _dTable[i]= 18+(i-'S');

	for(i= 'a';i<='i';i++) _dTable[i]= 26+(i-'a'); 

	for(i= 'j';i<='r';i++) _dTable[i]= 35+(i-'j');

	for(i= 's';i<='z';i++) _dTable[i]= 44+(i-'s');

	for(i= '0';i<='9';i++) _dTable[i]= 52+(i-'0');

	_dTable['+']= 62;
	_dTable['/']= 63;
	_dTable['=']= 0;

	_maxLineLength = 76 / 4;
}

Base64::~Base64() {
}

void Base64::Encode(
	const unsigned char *input, size_t n, string &output
) {
	output.reserve(GetEncodeSize(n));
	output.clear();

	int ngrps = 0;	// num output groups for current line

	int	ictr;
	for(ictr=0; ictr<(n-2); ictr+=3) {

		output.push_back( _eTable[input[ictr+0]>>2] );
		output.push_back( _eTable[((input[ictr+0]&3)<<4)|(input[ictr+1]>>4)]);
		output.push_back( _eTable[((input[ictr+1]&0xF)<<2)|(input[ictr+2]>>6)]);
		output.push_back( _eTable[input[ictr+2]&0x3F] );

		ngrps++;

		if (ngrps >= _maxLineLength) {
			output.push_back('\n');
			ngrps = 0;
		}
	}

	if (n-ictr) {
		output.push_back( _eTable[input[ictr+0]>>2] );
		if (n-ictr == 1) {
			output.push_back(_eTable[((input[ictr+0]&3)<<4)]);
			output.push_back('=');
			output.push_back('=');
		}
		else {	// n-ictr == 2
			output.push_back(_eTable[((input[ictr+0]&3)<<4)|(input[ictr+1]>>4)]);
			output.push_back(_eTable[((input[ictr+1]&0xF)<<2)]);
			output.push_back('=');
		}
	}
}

void Base64::EncodeStreamBegin(
	string &output
) {
	output.clear();
	_ngrps = 0;
	_inbufctr = 0;
}

void Base64::EncodeStreamNext(
	const unsigned char *input, size_t n, string &output
) {

	for(int i=0; i<n; i++) { 

		_inbuf[_inbufctr++] = input[i];

		if (_inbufctr == 3) {

			output.push_back( _eTable[_inbuf[0]>>2] );
			output.push_back( _eTable[((_inbuf[0]&3)<<4)|(_inbuf[1]>>4)]);
			output.push_back( _eTable[((_inbuf[1]&0xF)<<2)|(_inbuf[2]>>6)]);
			output.push_back( _eTable[_inbuf[2]&0x3F] );

			_inbufctr = 0;

			_ngrps++;
			if (_ngrps >= _maxLineLength) {
				output.push_back('\n');
				_ngrps = 0;
			}
		}
	}
	_ngrps = 0;
	_inbufctr = 0;
}

void Base64::EncodeStreamEnd(
	string &output
) {
	assert (_inbufctr < 3);

	if (_inbufctr) {
		output.push_back( _eTable[_inbuf[0]>>2] );
		if (_inbufctr == 1) {
			output.push_back(_eTable[((_inbuf[0]&3)<<4)]);
			output.push_back('=');
			output.push_back('=');
		}
		else {	// _inbufctr == 2
			output.push_back(_eTable[((_inbuf[0]&3)<<4)|(_inbuf[1]>>4)]);
			output.push_back(_eTable[((_inbuf[1]&0xF)<<2)]);
			output.push_back('=');
		}
	}

}

size_t Base64::GetEncodeSize(size_t n) {

	size_t noutchars;

	noutchars = (n+2) / 3 * 4;
	noutchars = noutchars + (noutchars / (_maxLineLength * 4));
	return(noutchars);
}

int Base64::Decode(
	const string &input, unsigned char *output, size_t *n
) {
	int	ictr;
	int	c = 0;
	unsigned char b[4];
	int octr = 0;
	int	bctr = 0;
	

	for (ictr=0; ictr<input.length(); ictr++) { 
		c = input[ictr];

		if (c <= ' ') continue;
		if (c == '=') break;

		if (_dTable[c] & 0x80) {
			SetErrMsg("Illegal input character at offset %d : '%c'", ictr, c);
			return(-1);
		}

		b[bctr++] = _dTable[c];

		if (bctr == 4) {
			output[octr++]= (b[0]<<2)|(b[1]>>4);
			output[octr++]= (b[1]<<4)|(b[2]>>2);
			output[octr++]= (b[2]<<6)|b[3];
			bctr = 0;
		}
	}

	if (bctr && c == '=') {
		if (! (bctr == 2 || bctr == 3)) {
			SetErrMsg("Illegal input character at offset %d : '%c'", ictr, c);
			return(-1);
		}

		output[octr++]= (b[0]<<2)|(b[1]>>4);
		if (bctr == 3) {
			output[octr++]= (b[1]<<4)|(b[2]>>2);
		}
	}

	for(; ictr<input.length(); ictr++) {
		c = input[ictr];
		if (! (c <= ' ' || c == '=')) {
			SetErrMsg("Illegal input character at offset %d : '%c'", ictr, c);
			return(-1);
		}
	}
	*n = octr;
	return(0);
}

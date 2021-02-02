/*
 Tests out GLaDOS string handling outside the kernel, 
 where we have much better debug tools.
*/
#include <iostream>
#define GLaDOS_HOSTED 1   /* standalone, don't redefine compiler's datatypes */
#define GLaDOS_IMPLEMENT_STRING 1  
#include "GLaDOS/GLaDOS.h"


/* Print an ordinary ASCII string */
void print(const StringSource &str) {
  auto xf=xform('\n',"\r\n",str);
  
  vector<CHAR16> wide=CHAR16_from_String(xf);
  
  // Call cout to actually print the string
  for (CHAR16 c:wide)
	  std::cout<<(char)c;
}

int main() {
	StringSource fileData="This is APPS/DATA.DAT: Read success!\n";
	print("File contents: "+fileData);
	return 0;
}


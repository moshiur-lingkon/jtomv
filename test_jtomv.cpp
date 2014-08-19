#include<stdio.h>
#include<string.h>
#include<math.h>
#include<ctype.h>
#include<assert.h>
#include<stdlib.h>
#include<time.h>
#include<assert.h>

#include<vector>
#include<queue>
#include<stack>
#include<map>
#include<set>
#include<iostream>
#include<algorithm>
#include<string>

#include "jtomv.h"

using namespace std;

void PrintJson(Json& json)
{
	string desc = "";
	json.ToStr(desc);
	puts(desc.c_str());
}

void test1()
{
	const char* str = "[345, 3455]";	
	Json json(str);
	PrintJson(json);

	Json json2("[\"abcd\", {\"xyz\": null  }, 645645, 34.34e3   ]");
	PrintJson(json2);	

	vector<Json>* v = json.GetVector();
	v->push_back(json2);

	PrintJson(json);
}

void manual_test()
{
	char buff[1024];
	while(true){
		gets(buff);
		if(strcmp(buff,"exit") == 0) break;

		Json json(buff);
		string str;
		json.ToStr(str);
		printf("%s\n",str.c_str());
	}
}

int main() {
	test1();
	manual_test();
	return 0;
}


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

int test_count = 0;

bool __assert_passed(const char* msg, bool cond)
{
	if(cond){
		printf("TEST#%d PASSED:[%s]\n", ++test_count, msg);
	}

	return cond;
}

void __assert_failed(const char* msg)
{
	printf("TEST#%d FAILED:[%s]\n", ++test_count, msg);
}

#define ASSERT(EX) (__assert_passed(#EX, (EX)) || (__assert_failed(#EX),0))

int main() {
	const char* str = "{\"key1\" : \"value1\"  }";
	Json json(str);		
	Json json2("\"abcd\"");	
	printf("%d\n", json2.GetType());
	ASSERT(json2.GetType() == JSON_TYPE_STRING);
	printf("%d\n", json.GetType());
	ASSERT(json.GetType() == JSON_TYPE_OBJECT);
	map<string, Json> m = *((map<string,Json>*)json.GetValue());	
	printf("m.size() = %d\n", m.size());

	//string s = *((string*)m["key1"].GetValue());
	//cout<<s<<endl;
	return 0;
}


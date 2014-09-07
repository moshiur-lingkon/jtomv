#include "jtomv.h"
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cassert>

using namespace std;
using namespace jtomv;

/*
 * The CFG for Parsing the Json is described here:
 * JSON 			= JSON_OBJECT | JSON_ARRAY | STRING | BOOLEAN | INTEGER | DOUBLE | NULL
 * JSON_OBJECT 			= { LIST_OF_KEY_VALUE_PAIR }
 * LIST_OF_KEY_VALUE_PAIR 	= <empty> | KEY_VALUE_PAIR | KEY_VALUE_PAIR , LIST_OF_KEY_VALUE_PAIR
 * KEY_VALUE_PAIR 		= STRING : JSON
 * JSON_ARRAY 			= [ LIST_OF_JSON ]
 * LIST_OF_JSON		 	= <empty> | JSON | JSON, LIST_OF_JSON
 * STRING 			= "<list-of-ascii-char>"
 * BOOLEAN 			= true | false
 * INTEGER 			= <list-of-digits>
 * DOUBLE 			= INTEGER MID END
 * MID 				= <empty> | . INTEGER
 * END 				= <empty> | e INTEGER
 */

// Recursive (read Boring) descent parser follows

#define REACHED_END (m_nPos >= m_JsonString.size())
#define BACKUP int pos = m_nPos
#define BACKUP2 pos = m_nPos
#define RESTORE m_nPos = pos
#define CUR_CHAR (m_JsonString[m_nPos])
#define SKIP_SPACE while ( !REACHED_END && isspace(CUR_CHAR) ) ++m_nPos

// NFA for checking double
struct Edge {
	int u, v;
	std::string alpha;
};

struct Nfa {
	std::vector<Edge> edgeList;
	int nodes;
};

static Nfa double_nfa;
static bool double_nfa_prepared = false;
static void prepare_double_nfa()
{
#define EDGE(X,Y,s) {E.u=(X);E.v=(Y);E.alpha=s; double_nfa.edgeList.push_back(E);}
	double_nfa.nodes = 12;
	Edge E;
	std::string _digits = "0123456789";
	EDGE(0,1,"");
	EDGE(0,1,"-");
	EDGE(1,2,"0");
	EDGE(1,5,"123456789");
	EDGE(2,3,".");
	EDGE(2,4,"");
	EDGE(3,3,_digits);
	EDGE(3,4,"");
	EDGE(4,6,"");
	EDGE(4,11,"");
	EDGE(5,5,_digits);
	EDGE(5,2,"");
	EDGE(6,7,"eE");
	EDGE(7,8,"");
	EDGE(8,9,"");
	EDGE(8,9,"+-");
	EDGE(9,10,"");
	EDGE(9,10,_digits);
	EDGE(10,11,"");
}

static bool Match(const Nfa& nfa, int at, const std::string& str, int pos)
{
	if (pos == str.size() && at == nfa.nodes - 1){
		return true;
	}

	for (int i = 0; i < nfa.edgeList.size(); ++i){
		if(nfa.edgeList[i].u == at){
			if(nfa.edgeList[i].alpha.empty()){
				if(Match(nfa, nfa.edgeList[i].v, str, pos)) return true;
			}
			else if(pos < str.size() && nfa.edgeList[i].alpha.find(str[pos]) != std::string::npos){
				if(Match(nfa, nfa.edgeList[i].v, str, pos+1)) return true;
			}
		}
	}

	return false;
}

void Json::Clear()
{
	switch (m_type){
		case JSON_TYPE_VECTOR:
		{
			vector<Json>* ptr = (vector<Json>*) m_pValue;
			for (int i = 0; i < ptr->size(); ++i)
				ptr->at(i).Clear();
		}
		break;

		case JSON_TYPE_MAP:
		{
			map<string,Json>* ptr = (map<string,Json>*) m_pValue;
			for (map<string,Json>::iterator it = ptr->begin(); it != ptr->end(); ++it)
				it->second.Clear();
		}
		break;

		case JSON_TYPE_STRING:
		{
			string* ptr = (string*)m_pValue;
			delete ptr;
		}
		break;

		case JSON_TYPE_BOOL:
		{
			bool* ptr = (bool*)m_pValue;
			delete ptr;
		}
		break;

		case JSON_TYPE_INT:
		{
			INT64* ptr = (INT64*)m_pValue;
			delete ptr;
		}
		break;

		case JSON_TYPE_DOUBLE:
		{
			double* ptr = (double*)m_pValue;
			delete ptr;
		}
		break;

		case JSON_TYPE_NULL:
		{
		}
		break;

		case JSON_TYPE_INVALID:
		{
		}
		break;
	};
}

Json::~Json()
{
	Clear();
}

bool Json::ParseChar(char ch)
{
	while ( !REACHED_END && isspace(CUR_CHAR) ) ++m_nPos;
	if(REACHED_END) return false;		
	bool ret = CUR_CHAR == ch;
	if(ret)
		++m_nPos;
	return ret;
}

bool Json::ParseKEY_VALUE_PAIR(std::map<std::string, Json>& jsonMap)
{
	std::string key;
	Json jKey;
	if ( !ParseSTRING(&jKey) ){
		return false;
	}
	key = *((string*)jKey.m_pValue);
	if ( !ParseChar(':') ){
		return false;
	}

	Json* jVal = new Json();
	if ( !ParseJSON(jVal) ){
		delete jVal;
		return false;
	}

	jsonMap[key] = *jVal;

	return true;
}

bool Json::ParseLIST_OF_KEY_VALUE_PAIR(std::map<std::string, Json>& jsonMap)
{
	BACKUP;
	if ( !ParseKEY_VALUE_PAIR(jsonMap) ){
		RESTORE;
		return true;
	}
	
	BACKUP2;
	if(!ParseChar(',')){
		RESTORE;
		return true;
	}

	if(!ParseLIST_OF_KEY_VALUE_PAIR(jsonMap)){
		jsonMap.clear();
		return false;
	}

	return true;
}

bool Json::ParseJSON_OBJECT(Json* res)
{
	if ( !ParseChar('{') ){
		return false;
	}

	std::map<string, Json> *jsonMap = new std::map<string, Json>();

	if ( !ParseLIST_OF_KEY_VALUE_PAIR(*jsonMap) ){
		delete jsonMap;
		return false;
	}

	if( !ParseChar('}') ){
		delete jsonMap;
		return false;
	}

	res->m_type = JSON_TYPE_MAP;
	res->m_pValue = jsonMap;

	return true;
}

bool Json::ParseSTRING(Json* res)
{
	if ( !ParseChar('"') ){
		return false;
	}
	std::string *str = new string("");

	while ( !REACHED_END && !(m_JsonString[m_nPos-1] != '\\' && CUR_CHAR == '"') ){
		if ( !(CUR_CHAR == '\\' && m_nPos+1 < m_JsonString.size() && m_JsonString[m_nPos+1] == '"') ){
			str->push_back(CUR_CHAR);
		}
		++m_nPos;
	}

	if ( REACHED_END ){
		delete str;
		return false;
	}

	m_nPos++; // skip the ending "

	res->m_type = JSON_TYPE_STRING;
	res->m_pValue = str;

	return true;
}

bool Json::MatchPrefix(const char* str)
{
	for (; *str; ++str){
		if (REACHED_END || CUR_CHAR != *str)
			return false;
		++m_nPos;
	}	
	return true;
}

bool Json::ParseBOOLEAN(Json* res)
{
	SKIP_SPACE;

	if (MatchPrefix("true")){
		res->m_type = JSON_TYPE_BOOL;
		res->m_pValue = new bool(true);
	}
	else if(MatchPrefix("false")){
		res->m_type = JSON_TYPE_BOOL;
		res->m_pValue = new bool(false);
	}
	else{
		return false;
	}

	return true;
}

bool Json::ParseINTEGER(Json* res)
{
	SKIP_SPACE;

	if ( REACHED_END ){
		return false;
	}

	int nBackup = m_nPos;
	int sign = 1;
	if(CUR_CHAR == '-'){
		sign = -1;
		++m_nPos;
	}

	BACKUP;
	INT64 *pnVal = new INT64();
	*pnVal = 0;
	while ( !REACHED_END && '0' <= CUR_CHAR && CUR_CHAR <= '9' ){
		*pnVal = *pnVal * 10 + (CUR_CHAR - '0');
		++m_nPos;
	}
	
	if(pos == m_nPos){ // at least one digit required
		delete pnVal;
		return false;	
	}

	if(CUR_CHAR == '.' || CUR_CHAR == 'e' || CUR_CHAR == 'E') { // it's probably a double
		delete pnVal;
		return false;
	}
	*pnVal *= sign;
	res->m_type = JSON_TYPE_INT;
	res->m_pValue = pnVal;

	return true;
}

bool Json::ParseDOUBLE(Json* res)
{
	if ( !double_nfa_prepared ){
		prepare_double_nfa();
		double_nfa_prepared = true;
	}

	SKIP_SPACE;
	std::string sToken;
	while ( !REACHED_END ) {
		if ( CUR_CHAR != '+' && CUR_CHAR != '-' && CUR_CHAR != '.' && CUR_CHAR != 'e' && CUR_CHAR != 'E' && !isdigit(CUR_CHAR) )
			break;
		else
			sToken += CUR_CHAR;
		++m_nPos;
	}
	if(!Match(double_nfa, 0, sToken, 0)){
		return false;
	}

	res->m_type = JSON_TYPE_DOUBLE;
	res->m_pValue = new double(atof(sToken.c_str()));

	return true;
}

bool Json::ParseNULL(Json* res)
{
	SKIP_SPACE;

	if(MatchPrefix("null")){
		res->m_type = JSON_TYPE_NULL;
		res->m_pValue = NULL;
		return true;
	}

	return false;
}

bool Json::ParseLIST_OF_JSON(vector<Json>& vjson)
{
	BACKUP;

	Json* json = new Json();			
	if( !ParseJSON(json) ){
		RESTORE;
		delete json;
		return true;
	}
	vjson.push_back(*json);
	BACKUP2;
	if(!ParseChar(',')){
		RESTORE;	
		return true;
	}	

	if(!ParseLIST_OF_JSON(vjson)){
		delete json;
		vjson.clear();
		return false;
	}

	return true;
}

bool Json::ParseJSON_ARRAY(Json* res)
{
	if( !ParseChar('[')){
		return false;
	}
	vector<Json>* vjson = new vector<Json>();
	if(!ParseLIST_OF_JSON(*vjson)){
		delete vjson;
		return false;
	}

	if(!ParseChar(']')){
		delete vjson;
		return false;
	}

	res->m_type = JSON_TYPE_VECTOR;
	res->m_pValue = vjson;
	return true;
}

bool Json::ParseJSON(Json* res)
{
	BACKUP;
	if ( ParseJSON_OBJECT(res) )
		return true;
	RESTORE;
	if ( ParseJSON_ARRAY(res) )
		return true;
	RESTORE;
	if ( ParseSTRING(res) ){
		return true;
	}
	RESTORE;
	if ( ParseBOOLEAN(res) )
		return true;
	RESTORE;
	if ( ParseINTEGER(res) ){
		return true;
	}
	RESTORE;
	if ( ParseDOUBLE(res) )
		return true;
	RESTORE;
	if ( ParseNULL(res) )
		return true;
	RESTORE;
	res->m_type = JSON_TYPE_INVALID;
	res->m_pValue = NULL;
	return false;
}

JsonType Json::GetType()
{
	return m_type;
}

std::vector<Json>* Json::GetVector()
{
	return m_type == JSON_TYPE_VECTOR ? (std::vector<Json>*)m_pValue : NULL;
}

std::map<std::string, Json>* Json::GetMap()
{
	return m_type == JSON_TYPE_NULL ? (std::map<std::string, Json>*)m_pValue : NULL;
}

std::string* Json::GetString()
{
	return m_type == JSON_TYPE_STRING ? (std::string*)m_pValue : NULL;
}

bool* Json::GetBool()
{
	return m_type == JSON_TYPE_BOOL ? (bool*)m_pValue : NULL;
}

INT64* Json::GetInt()
{
	return m_type == JSON_TYPE_INT ? (INT64*)m_pValue : NULL;
}

double* Json::GetDouble()
{
	return m_type == JSON_TYPE_DOUBLE ? (double*)m_pValue : NULL;
}

Json::Json()
{
	m_type = JSON_TYPE_INVALID;
	m_pValue = NULL;
}

void Json::DeepCopy(const Json& obj)
{
	m_type = obj.m_type;
	switch(obj.m_type){
		case JSON_TYPE_VECTOR:
			{
				vector<Json>* v = new vector<Json>();
				*v = *((vector<Json>*)obj.m_pValue);
				m_pValue = v;
			}
			break;
		case JSON_TYPE_MAP:
			{
				map<string,Json>* m = new map<string,Json>();
				*m = *((map<string,Json>*)obj.m_pValue);
				m_pValue = m;
			}
			break;
		case JSON_TYPE_STRING:
			{
				string* s = new string();
				*s = *((string*)obj.m_pValue);
				m_pValue = s;
			}
			break;
		case JSON_TYPE_BOOL:
			{
				bool *b = new bool();
				*b = *((bool*)obj.m_pValue);
				m_pValue = b;
			}
			break;
		case JSON_TYPE_INT:
			{
				INT64* n = new INT64();
				*n = *((INT64*)obj.m_pValue);
				m_pValue = n;
			}
			break;
		case JSON_TYPE_DOUBLE:
			{
				double* d = new double();
				*d = *((double*)obj.m_pValue);
				m_pValue = d;
			}
			break;
		case JSON_TYPE_NULL:
			{
				m_pValue = NULL;
			}
			break;
		case JSON_TYPE_INVALID:
			{
				m_pValue = NULL;
			}
			break;
	}
}

Json::Json(const Json& obj)
{
	if(this != &obj){
		DeepCopy(obj);
	}
}

Json& Json::operator = (const Json& obj)
{
	if(this != &obj){
		DeepCopy(obj);
	}
	return *this;
}

Json::Json(std::string jsonString)
{
	m_nPos = 0;
	m_JsonString = jsonString;
	
	Json* json = new Json();
	if ( !ParseJSON(json) ){
		delete json;
		m_type = JSON_TYPE_INVALID;
		m_pValue = NULL;
	}
	else{
		m_type = json->m_type;
		m_pValue = json->m_pValue;
	}

	SKIP_SPACE;
	if(!REACHED_END){
		Clear();
		m_type = JSON_TYPE_INVALID;
		m_pValue = NULL;
	}
}

void Json::ToStr(std::string& str)
{
	switch (m_type){
		case JSON_TYPE_VECTOR:
		{
			str.push_back('[');
			vector<Json>* v = (vector<Json>*)m_pValue;
			bool flag = false;
			for (vector<Json>::iterator it = v->begin(); it != v->end(); ++it){
				if(flag) str.push_back(',');
				it->ToStr(str);
				flag = true;
			}
			str.push_back(']');
			break;
		}

		case JSON_TYPE_MAP:
		{
			str.push_back('{');
			map<string,Json>* m = (map<string,Json>*)m_pValue;
			bool flag = false;
			for(map<string,Json>::iterator it=m->begin(); it!=m->end(); ++it){
				if(flag) str.push_back(',');
				str += "\"" + it->first + "\"";
				str.push_back(':');
				it->second.ToStr(str);
				flag = true;
			}	
			str.push_back('}');	
			break;
		}

		case JSON_TYPE_STRING:
		{
			str.push_back('"');
			str += *((string*)m_pValue);
			str.push_back('"');
			break;
		}

		case JSON_TYPE_BOOL:
		{
			if(*((bool*)m_pValue))
				str += "true";
			else
				str += "false";
			break;
		}

		case JSON_TYPE_INT:
		{
			INT64 n = *((INT64*)m_pValue);
			int sign = 1;
			if(n < 0){
				sign = -1;
				n = -n;
			}

			string buff = "";
			if(n == 0) buff.push_back('0');
			while (n > 0){
				buff.push_back('0' + n%10);
				n /= 10;
			}
			if(sign == -1) buff.push_back('-');
			reverse(buff.begin(), buff.end());
			str += buff;
			break;
		}

		case JSON_TYPE_DOUBLE:
		{
			char buff[64];
			sprintf(buff, "%lf", *((double*)m_pValue));
			str += buff;
			break;
		}

		case JSON_TYPE_NULL:
		{
			str += "null";
			break;
		}

		case JSON_TYPE_INVALID:
		{
			str = "invalid";
			break;
		}
	}
}


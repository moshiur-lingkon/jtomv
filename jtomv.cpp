#include "jtomv.h"
#include <vector>
#include <cstdio>
#include <cstdlib>

using namespace std;

/*
 * The CFG for Parsing the Json is described here:
 * JSON 			= JSON_OBJECT | JSON_ARRAY | STRING | BOOLEAN | INTEGER | DOUBLE | NULL
 * JSON_OBJECT 			= { LIST_OF_KEY_VALUE_PAIR }
 * LIST_OF_KEY_VALUE_PAIR 	= <empty> | KEY_VALUE_PAIR | KEY_VALUE_PAIR , LIST_OF_KEY_VALUE_PAIR
 * KEY_VALUE_PAIR 		= STRING : VALUE
 * VALUE 			= JSON
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
	if (pos == str.size() && at == nfa.nodes -1) return true;

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

Json::Json()
{
	m_type = JSON_TYPE_INVALID;
	m_pValue = NULL;
}

void Json::Clear()
{
	switch (m_type){
		case JSON_TYPE_ARRAY:
		{
			vector<Json>* ptr = (vector<Json>*) m_pValue;
			for (int i = 0; i < ptr->size(); ++i)
				ptr->at(i).Clear();
		}
		break;

		case JSON_TYPE_OBJECT:
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

		case JSON_TYPE_BOOLEAN:
		{
			bool* ptr = (bool*)m_pValue;
			delete ptr;
		}
		break;

		case JSON_TYPE_INTEGER:
		{
			int* ptr = (int*)m_pValue;
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
	while ( !REACHED_END && CUR_CHAR != ch ) ++m_nPos;
	return !REACHED_END; // implies CUR_CHAR == ch
}

bool Json::ParseKEY_VALUE_PAIR(std::map<std::string, Json>& jsonMap)
{
	BACKUP;
	std::string key;
	Json jKey;
	if ( !ParseSTRING(jKey) ){
		RESTORE;
		return false;
	}
	key = *((string*)jKey.m_pValue);
	if ( !ParseChar(':') ){
		RESTORE;
		return false;
	}

	Json json;
	if ( !ParseJSON(json) ){
		RESTORE;
		return false;
	}

	jsonMap[key] = json;
}

bool Json::ParseLIST_OF_KEY_VALUE_PAIR(std::map<std::string, Json>& jsonMap)
{
	BACKUP;

	if ( ParseChar('}') ){
		return true;
	}

	RESTORE;

	if ( !ParseKEY_VALUE_PAIR(jsonMap) ){
		RESTORE;
		return false;
	}

	if ( !ParseChar(',') ){
		RESTORE;
		if ( !ParseChar('}') ){
			m_nPos = pos;
			jsonMap.clear();
			return false;
		}
		else{
			return true;
		}
	}

	if ( !ParseLIST_OF_KEY_VALUE_PAIR(jsonMap) ){
		jsonMap.clear();
		RESTORE;
		return false;
	}

	return true;
}

bool Json::ParseJSON_OBJECT(Json& res)
{
	BACKUP;
	if ( !ParseChar('{') ){
		RESTORE;
		return false;
	}

	std::map<string, Json> *jsonMap = new std::map<string, Json>();

	if ( !ParseLIST_OF_KEY_VALUE_PAIR(*jsonMap) ){
		RESTORE;
		delete jsonMap;
		return false;
	}

	if( !ParseChar('}') ){
		RESTORE;
		delete jsonMap;
		return false;
	}

	res.m_type = JSON_TYPE_OBJECT;
	res.m_pValue = jsonMap;

	return true;
}

bool Json::ParseSTRING(Json& res)
{
	BACKUP;
	if ( !ParseChar('"') ){
		RESTORE;
		return false;
	}
	std::string *str = new string();

	while ( !REACHED_END && !(m_JsonString[m_nPos-1] != '\\' && CUR_CHAR == '"') ){
		if ( !(CUR_CHAR == '\\' && m_nPos+1 < m_JsonString.size() && m_JsonString[m_nPos+1] == '"') ){
			str->push_back(CUR_CHAR);
		}
		++m_nPos;
	}

	if ( m_nPos == m_JsonString.size() ){
		delete str;
		RESTORE;
		return false;
	}

	m_nPos++; // skip the ending "

	res.m_type = JSON_TYPE_STRING;
	res.m_pValue = str;

	return true;
}

bool Json::ParseBOOLEAN(Json& res)
{
	BACKUP;
	SKIP_SPACE;

	std::string token;
	while ( !REACHED_END && !isspace(CUR_CHAR) )
		token += m_JsonString[m_nPos++];
	if ( token == "true" ){
		res.m_type = JSON_TYPE_BOOLEAN;
		res.m_pValue = new bool(true);
	}
	else if( token == "false" ){
		res.m_type = JSON_TYPE_BOOLEAN;
		res.m_pValue = new bool(false);
	}
	else{
		RESTORE;
		return false;
	}

	return true;
}

bool Json::ParseINTEGER(Json& res)
{
	BACKUP;
	SKIP_SPACE;

	if ( REACHED_END ){
		RESTORE;
		return false;
	}

	INT64 val = 0;
	while ( !REACHED_END && '0' <= CUR_CHAR && CUR_CHAR <= '9' ){
		val = val * 10 + (CUR_CHAR - '0');
	}
	
	if(pos == m_nPos){
		RESTORE;
		return false;	
	}

	res.m_type = JSON_TYPE_INTEGER;
	res.m_pValue = new INT64(val);

	return true;
}

bool Json::ParseDOUBLE(Json& res)
{
	BACKUP;
	SKIP_SPACE;
	std::string sToken;
	while ( !REACHED_END ) {
		if ( CUR_CHAR != '+' && CUR_CHAR != '-' && CUR_CHAR != '.' && CUR_CHAR != 'e' && CUR_CHAR != 'E' &&!isdigit(CUR_CHAR) )
			break;
		else
			sToken += CUR_CHAR;
		++m_nPos;
	}

	if(!Match(double_nfa, 0, sToken, 0)){
		RESTORE;
		return false;
	}

	res.m_type = JSON_TYPE_DOUBLE;
	res.m_pValue = new double(atof(sToken.c_str()));

	return true;
}

bool Json::ParseNULL(Json& res)
{
	BACKUP;
	SKIP_SPACE;

	std::string token;
	while ( !REACHED_END && !isspace(CUR_CHAR) )
		token += m_JsonString[m_nPos++];

	if(token == "null"){
		res.m_type = JSON_TYPE_NULL;
		res.m_pValue = NULL;
		return true;
	}

	RESTORE;
	return false;
}

bool Json::ParseLIST_OF_JSON(vector<Json>& vjson)
{
	BACKUP;
	if( ParseChar(']') ){
		RESTORE;
		return true;
	}	
	Json json;			
	if( !ParseJSON(json) ){
		RESTORE;
		vjson.clear();
		return false;
	}
	vjson.push_back(json);
	if(!ParseChar(',')){
		if(!ParseChar(']')){
			RESTORE;
			vjson.clear();
			return false;
		}
		else{
			return true;
		}
	}	
	if(!ParseLIST_OF_JSON(vjson)){
		RESTORE;
		vjson.clear();
		RESTORE;
	}

	return true;
}

bool Json::ParseJSON_ARRAY(Json& res)
{
	BACKUP;
	if( !ParseChar('[')){
		RESTORE;
		return false;
	}
	vector<Json>* vjson = new vector<Json>();
	if(!ParseLIST_OF_JSON(*vjson)){
		delete vjson;
		RESTORE;
		return false;
	}

	if(!ParseChar(']')){
		delete vjson;
		RESTORE;
		return false;
	}

	res.m_type = JSON_TYPE_ARRAY;
	res.m_pValue = vjson;
	return true;
}

bool Json::ParseJSON(Json& res)
{
	BACKUP;
	if ( ParseJSON_OBJECT(res) )
		return true;
	RESTORE;
	if ( ParseJSON_ARRAY(res) )
		return true;
	RESTORE;
	if ( ParseSTRING(res) )
		return true;
	RESTORE;
	if ( ParseBOOLEAN(res) )
		return true;
	RESTORE;
	if ( ParseDOUBLE(res) )
		return true;
	RESTORE;
	if ( ParseINTEGER(res) )
		return true;
	RESTORE;
	if ( ParseNULL(res) )
		return true;
	RESTORE;
	return false;
}

JsonType Json::GetType()
{
	return m_type;
}

void* Json::GetValue()
{
	return m_pValue;	
}

Json::Json(std::string jsonString)
{
	if ( !double_nfa_prepared ){
		prepare_double_nfa();
		double_nfa_prepared = true;
	}

	m_nPos = 0;
	m_JsonString = jsonString;

	if ( !ParseJSON(*this) ){
		m_type = JSON_TYPE_INVALID;
		m_pValue = NULL;
	}
}


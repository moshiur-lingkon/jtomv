#include "JsonVectorMap.h"
#include <string>

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
#define RESTORE m_npos = pos

Json::Json()
{
	
}

bool Json::ParseChar(char ch)
{
	while ( !REACHED_END && isspace(m_JsonString[m_nPos]) )
		++m_nPos;
	return !REACHED_END && m_JsonString[m_nPos++] == ch;
}

bool Json::ParseKEY_VALUE_PAIR(std::map<std::string, Json>& jsonMap)
{
	int pos = m_nPos;	
	std::string key;

	if ( !ParseSTRING(key) ){
		m_nPos = pos;
		return false;
	}

	if ( !ParseChar(':') ){
		m_nPos = pos;
		return false
	}

	Json json;
	if ( !ParseJSON(json) ){
		m_nPos = pos;
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

	if ( !ParseLIST_OF_KEY_VALUE_PAIR() ){
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
		return false;
	}

	if( !ParseChar('}') ){
		RESTORE;
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

	while ( !REACHED_END && !(m_JsonString[m_nPos-1] != '\\' && m_JsonString[m_nPos] == '"') ){	
		if ( !(m_JsonString[m_nPos] == '\\' && m_nPos+1 < m_JsonString.size() && m_JsonString[m_nPos+1] == '"') ){
			str->push_back(m_JsonString[m_nPos]);
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
	while ( !REACHED_END && isspace(m_JsonString[m_nPos]) )
		++m_nPos;
	

	std::string token;
	while ( !REACHED_END && !isspace(m_JsonString[m_nPos]) )
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
	
	while ( !REACHED_END && isspace(m_JsonString[m_nPos] )
		m_JsonString[m_nPos++];	

	if ( REACHED_END ){
		RESTORE;
		return false;
	}

	INT64 val = 0;
	while ( !REACHED_END && '0' <= m_JsonString[m_nPos] && m_JsonString[m_nPos] <= '9' ){
		val = val * 10 + (m_JsonString[m_nPos] - '0');
	}

	if ( REACHED_END ) {
		RESTORE;
		return false;
	}

	res.m_type = JSON_TYPE_INTEGER;	
	res.m_pValue = new INT64(val);

	return true;
}

bool Json::ParseDOUBLE(Json& res)
{

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
	if ( ParseINTEGER(res) )
		return true;
	RESTORE;
	if ( ParseDOUBLE(res) )
		return true;
	RESTORE;
	if ( ParseNULL(res) )
		return true;
	RESTORE;
	return false;
}

Json::Json(std::string jsonString)
{
	m_nPos = 0;
	m_type = parsed.first;
	m_pValue = parsed.second;
	m_JsonString = jsonString;
	
	ParsingResult val;

	if ( ParseJSON1(val) ){
		m_type = val.type;
		m_pValue = val.val;
	}
}


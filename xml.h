#ifndef XML_H
#define XML_H


string xmlStringToTag(string aTagName, string aValue);
string xmlBoolToTag(string aTagName, bool aValue);
string xmlIntToTag(string aTagName, int aValue);

string xmlStringArrayToTag(string aArrayTagName, string aElementTagName, vector<string> aArrayValues);

string xmlGetString(string aDataXML, string aTagName);
bool xmlGetBool(string aDataXML, string aTagName);
int xmlGetInt(string aDataXML, string aTagName);

string xmlDeleteTags(string aDataXML, string aTagName);
string xmlDeleteTag(string aDataXML, string aTagName);
string xmlDeleteTagById(string aDataXML, string aTagName, string aTagId);

vector<string> xmlGetArrayValues(string aDataXML, string aTagName);
vector<string> xmlGetArrayTags(string aDataXML, string aTagName);

string xmlGetTagById(string aDataXML, string aTagName, string aTagId);
string xmlGetTag(string aDataXML, string aTagName);
string xmlGetTags(string aDataXML, string aTagName);

string xmlSetTagById(string aDataXML, string aTagName, string aTagId, string aNewTag);

string xmlAddTag(string aDataXML, string aParentTagName, string aNewTag);
string xmlAddTagById(string aDataXML, string aParentTagName, std::string aParentTagId, string aNewTag);

string xmlSetString(string aDataXML, string aTagName, string aNewValue);
string xmlSetBool(string aDataXML, string aTagName, bool aNewValue);

string xmlSetStringById(string aDataXML, string aParentTagName, string aParentTagId, string aTargetTegName,  string aNewValue);
string xmlSetBoolById(string aDataXML, string aParentTagName, string aParentTagId, string aTargetTegName,  bool aNewValue);

#endif // XML_H

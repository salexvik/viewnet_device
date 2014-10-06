#include "server.h"  // мой заголовочныйный файл
#include "xml.h"


// Добавление строкового значения в строку формата XML

string xmlStringToTag(string aTagName, string aValue){
    string startTag = "<" + aTagName + ">";
    string endTag = "</" + aTagName + ">";
    return startTag + aValue + endTag;
}


// Добавление булевого значения в строку формата XML

string xmlBoolToTag(string aTagName, bool aValue){

    string startTag = "<" + aTagName + ">";
    string endTag = "</" + aTagName + ">";

    if (aValue) return startTag + "true" + endTag;
    else return startTag + "false" + endTag;

}


// Добавление строкового значения в строку формата XML

string xmlIntToTag(string aTagName, int aValue){
    string startTag = "<" + aTagName + ">";
    string endTag = "</" + aTagName + ">";
    return startTag + intToString(aValue) + endTag;
}



//  массив строковых значений в строку формата XML

string xmlStringArrayToTag(string aArrayTagName, string aElementTagName, vector<string> aArrayValues){

    string tempString = "";

    string startTag = "<" + aElementTagName + ">";
    string endTag = "</" + aElementTagName + ">";

    // проходим в цикле по строкам массива
    for(vector<string>::const_iterator it = aArrayValues.begin(); it !=aArrayValues.end(); ++it)
        tempString += startTag + *it + endTag;

    startTag = "<" + aArrayTagName + ">";
    endTag = "</" + aArrayTagName + ">";

    return startTag + tempString + endTag;
}



// Добавление тега в родительский тег

string xmlAddTag(string aDataXML, string aParentTagName, string aNewTag){

    string targetTagValue = xmlGetString(aDataXML, aParentTagName);
    string newValue = targetTagValue + aNewTag;

    return xmlSetString(aDataXML, aParentTagName, newValue);
}



// Добавление тега в родительский тег по идентификатору

string xmlAddTagById(string aDataXML, string aParentTagName, string aParentTagId, string aNewTag){

    string tempData = aDataXML;

    string targetTag = xmlGetTagById(tempData, aParentTagName, aParentTagId);

    targetTag = xmlAddTag(targetTag, aParentTagName, aNewTag);

    return xmlSetTagById(tempData, aParentTagName, aParentTagId, targetTag);
}




// Получение строкового значения из строки в формате XML

string xmlGetString(string aDataXML, string aTagName){

    string startTag = "<" + aTagName + ">";
    string endTag = "</" + aTagName + ">";
    int posStartTag = 0;
    int posEndTag = 0;
    string value = "";

    posStartTag = aDataXML.find(startTag);
    if(posStartTag == -1) return "";
    posEndTag = aDataXML.find(endTag);
    if(posEndTag == -1) return "";
    value = aDataXML.substr(posStartTag + startTag.length(), posEndTag - (posStartTag + startTag.length()));
    return value;
}


// Получение булевого значения из строки в формате XML

bool xmlGetBool(string aDataXML, string aTagName){

    string startTag = "<" + aTagName + ">";
    string endTag = "</" + aTagName + ">";
    int posStartTag = 0;
    int posEndTag = 0;
    string value = "";

    posStartTag = aDataXML.find(startTag);
    if(posStartTag == -1) return false;
    posEndTag = aDataXML.find(endTag);
    if(posEndTag == -1) return false;
    value = aDataXML.substr(posStartTag + startTag.length(), posEndTag - (posStartTag + startTag.length()));

    if (value == "true") return true;
    else return false;
}


// Получение цифрового значения из строки в формате XML (int)

int xmlGetInt(string aDataXML, string aTagName){

    string startTag = "<" + aTagName + ">";
    string endTag = "</" + aTagName + ">";
    int posStartTag = 0;
    int posEndTag = 0;
    string value = "";

    posStartTag = aDataXML.find(startTag);
    if(posStartTag == -1) return -1;
    posEndTag = aDataXML.find(endTag);
    if(posEndTag == -1) return -1;
    value = aDataXML.substr(posStartTag + startTag.length(), posEndTag - (posStartTag + startTag.length()));
    return atoi(value.c_str());
}


// Получение массива строковых значений из данных в формате XML

vector<string> xmlGetArrayValues(string aDataXML, string aTagName){

    vector<string> arrayList;

    string startTag = "<" + aTagName + ">";
    string endTag = "</" + aTagName + ">";
    int posStartTag = 0;
    int posEndTag = 0;

    while((posStartTag = aDataXML.find(startTag)) != -1){
        posEndTag = aDataXML.find(endTag);
        if(posEndTag == -1) return arrayList;
        arrayList.push_back(aDataXML.substr(posStartTag + startTag.length(), posEndTag - (posStartTag + startTag.length())));
        aDataXML.erase(posStartTag, (posEndTag + endTag.length()) - posStartTag);
    }
    return arrayList;
}


// Получение массива тегов из данных в формате XML

vector<string> xmlGetArrayTags(string aDataXML, string aTagName){

    vector<string> arrayList;

    string startTag = "<" + aTagName + ">";
    string endTag = "</" + aTagName + ">";
    int posStartTag = 0;
    int posEndTag = 0;

    while((posStartTag = aDataXML.find(startTag)) != -1){
        posEndTag = aDataXML.find(endTag);
        if(posEndTag == -1) return arrayList;
        arrayList.push_back(aDataXML.substr(posStartTag, (posEndTag + endTag.length()) - posStartTag));
        aDataXML.erase(posStartTag, (posEndTag + endTag.length()) - posStartTag);
    }
    return arrayList;
}


// Получение тега по идентификатору

string xmlGetTagById(string aDataXML, string aTagName, string aTagId){

    vector<string> arrayList;

    arrayList = xmlGetArrayTags(aDataXML, aTagName);

    for(vector<string>::const_iterator it = arrayList.begin(); it !=arrayList.end(); ++it){

        string targetTag = *it;
        int posId = targetTag.find(aTagId);
        if(posId != -1) return  *it;
    }

    return "";
}


// Поллучение всех заданных тегов из строки

string xmlGetTags(string aDataXML, string aTagName){

    string result = "";
    string startTag = "<" + aTagName + ">";
    string endTag = "</" + aTagName + ">";
    int posStartTag = 0;
    int posEndTag = 0;

    while((posStartTag = aDataXML.find(startTag)) != -1){
        posEndTag = aDataXML.find(endTag);
        if(posEndTag == -1) return "";

        result += aDataXML.substr(posStartTag, (posEndTag + endTag.length()) - posStartTag);
        aDataXML.erase(posStartTag, (posEndTag + endTag.length()) - posStartTag);
    }
    return result;
}


// Получение первого найденного тега из строки

string xmlGetTag(string aDataXML, string aTagName){

    string result = "";
    string startTag = "<" + aTagName + ">";
    string endTag = "</" + aTagName + ">";
    int posStartTag = 0;
    int posEndTag = 0;

    posStartTag = aDataXML.find(startTag);
    if(posStartTag == -1) return "";
    posEndTag = aDataXML.find(endTag);
    if(posEndTag == -1) return "";

    result = aDataXML.substr(posStartTag, (posEndTag + endTag.length()) - posStartTag);

    return result;
}


// Удаление всех заданных тегов из строки в формате XML

string xmlDeleteTags(string aDataXML, string aTagName){

    string tempData = aDataXML;

    while(true){

        string tagToRemove = xmlGetTag(tempData, aTagName);
        if (tagToRemove == "") return tempData;
        int posTagToRemove = tempData.find(tagToRemove);
        if (posTagToRemove == -1) return tempData;
        tempData.erase(posTagToRemove, tagToRemove.length());
    }

    return tempData;
}


// Удаление первого заданного тега из строки в формате XML

string xmlDeleteTag(string aDataXML, string aTagName){

    string tempData = aDataXML;

    string targetTag = xmlGetTag(tempData, aTagName);
    if (targetTag == "") return tempData;
    int posTargetTag = tempData.find(targetTag);
    if (posTargetTag == -1) return tempData;
    tempData.erase(posTargetTag, targetTag.length());

    return tempData;
}


// Удаление тега по идентификатору

string xmlDeleteTagById(string aDataXML, string aTagName, string aTagId){

    string tempData = aDataXML;

    string targetTag = xmlGetTagById(tempData, aTagName, aTagId);
    if (targetTag == "") return "";
    int posTargetTag = tempData.find(targetTag);
    if (posTargetTag == -1) return "";
    tempData.erase(posTargetTag, targetTag.length());

    return tempData;
}





// Замена тега по идентификатору

string xmlSetTagById(string aDataXML, string aTagName, string aTagId, string aNewTag){

    string tempData = aDataXML;

    string targetTag = xmlGetTagById(tempData, aTagName, aTagId);
    if (targetTag == "") return "";
    int posTargetTag = tempData.find(targetTag);
    if (posTargetTag == -1) return "";

    string startData = tempData.substr(0, posTargetTag);
    string endData = tempData.substr(posTargetTag + targetTag.length());

    tempData = startData + aNewTag + endData;
    if (tempData == "") return "";

    return tempData;
}




// Установка строкового сначения в первом найденном теге

string xmlSetString(string aDataXML, string aTagName, string aNewValue){


    string startTag = "<" + aTagName + ">";
    string endTag = "</" + aTagName + ">";
    int posStartTag = 0;
    int posEndTag = 0;

    posStartTag = aDataXML.find(startTag);
    if(posStartTag == -1) return "";
    posEndTag = aDataXML.find(endTag);
    if(posEndTag == -1) return "";

    return aDataXML.substr(0, posStartTag + startTag.length()) + aNewValue + aDataXML.substr(posEndTag);
}


// Установка булевого значения в первом найденном теге

string xmlSetBool(string aDataXML, string aTagName, bool aNewValue){

    string startTag = "<" + aTagName + ">";
    string endTag = "</" + aTagName + ">";
    int posStartTag = 0;
    int posEndTag = 0;

    posStartTag = aDataXML.find(startTag);
    if(posStartTag == -1) return "";
    posEndTag = aDataXML.find(endTag);
    if(posEndTag == -1) return "";

    if (aNewValue) return aDataXML.substr(0, posStartTag + startTag.length()) + "true" + aDataXML.substr(posEndTag);
    if (!aNewValue) return aDataXML.substr(0, posStartTag + startTag.length()) + "false" + aDataXML.substr(posEndTag);

    return "";
}



// Установка строкового значения в теге по идентификатору родительского тега

string xmlSetStringById(string aDataXML, string aParentTagName, string aParentTagId, string aTargetTegName,  string aNewValue){

    string tempData = aDataXML;

    string parentTag = xmlGetTagById(tempData, aParentTagName, aParentTagId);
    if (parentTag == "") return "";

    string newParentTag = xmlSetString(parentTag, aTargetTegName, aNewValue);
    if (newParentTag == "") return "";

    return xmlSetTagById(tempData, aParentTagName, aParentTagId, newParentTag);
}


// Установка строкового значения в теге по идентификатору родительского тега

string xmlSetBoolById(string aDataXML, string aParentTagName, string aParentTagId, string aTargetTegName,  bool aNewValue){

    string tempData = aDataXML;

    string parentTag = xmlGetTagById(tempData, aParentTagName, aParentTagId);
    if (parentTag == "") return "";

    string newParentTag = xmlSetBool(parentTag, aTargetTegName, aNewValue);
    if (newParentTag == "") return "";

    return xmlSetTagById(tempData, aParentTagName, aParentTagId, newParentTag);
}







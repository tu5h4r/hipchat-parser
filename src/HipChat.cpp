//============================================================================
// Name        : HipChat.cpp
// Author      :  Tushar Singh
// Description : Solution to Atlassian take-home coding exercise to
//                       write a RESTful API that takes a chat message string
//                       as input and returns a JSON object containing information about its contents .
//============================================================================

#include <iostream>
#include <string>
#include <vector>

/*JSON is a lightweight data-interchange format.
 * It can represent numbers, strings, ordered sequences of values, and collections of name/value pairs.
 * JsonCpp is a C++ library that allows manipulating JSON values, including serialization and deserialization to and from strings.
*  Its an open source library - https://github.com/open-source-parsers/jsoncpp
*  This is an amalgamated form of the entire JSON library
*  The methodology to use JSON CPP over other C++ libraries is based on time complexities for parsing and serialization
*  The methodology results can be found here - https://blog.thousandeyes.com/efficiency-comparison-c-json-libraries
 */
#include "json/json.h"

/*C++ cURL library, also known as libcurl to fetch the HTML contents based on URL passed
 * An opensource/free and robust library with API details found at https://curl.haxx.se/libcurl/c/
 * */
#include <curl/curl.h>
#include <ctype.h>

using namespace std;
using namespace Json;


/*Description - The C++ class to Handle all the functions for parsing the chat strings for detecting mentions, links and emoticons
 *
 */
class ParseStringAPI{

  private:
    //Variable to hold the entire JSON object. m_parsedJSON["mentions"],m_parsedJSON["emoticons"]  and m_parsedJSON["links"] holds parsed info.
    Value m_parsedJSON;

    //Main function to perform string parsing
    void ParseInput(string input);

    //Function to invoke libcurl function for fetching the HTML content with the url
    string getCURLOp(string URL);

    //The libcurl CURLOPT_WRITEFUNCTION callback, that gets called after a successful curl fetch operation
    static size_t handle(char * contents, size_t size, size_t nmemb, string * s);

  public:

    //Constructor to initialize the fields of JSON object as value array, as a single string might contain multiple mentions, links and emoticons
    ParseStringAPI(){
      m_parsedJSON["mentions"] =  Value(arrayValue);
      m_parsedJSON["emoticons"] =  Value(arrayValue);
      m_parsedJSON["links"] =  Value(arrayValue);
    }

    //The API to get the JSOn object
    Value GetJSONMessage(string input);
};

/*Description - The libcurl CURLOPT_WRITEFUNCTION callback, that gets called after a successful curl fetch operation.
 *                        if the libcurl is not able to fetch the entire data in one call, the rest of data is piped in multiple callbacks.
 *                        This will happen if the curl fetch returns data more than CURL_MAX_WRITE_SIZE (the usual default is 16K)
 *                        defined in curl.h header file
 * Input - contents (The fetched HTML contents), size (size of each block fetched), nmemb (number of block fetched), userdata
 *              (string that will be used as user data to save the result defined in triggering function with CURLOPT_WRITEDATA set option)
 * Output - Total size of fetched block in single callback
*/

size_t ParseStringAPI::handle(char * contents, size_t size, size_t nmemb, string * userdata)
{
      size_t newLength = size*nmemb;
      size_t oldLength =userdata->size();
      try
      {
        userdata->resize(oldLength + newLength); //Resizing the user data if old length is non zero
      }
      catch(std::bad_alloc &e)
      {
        //handle memory problem
        return 0;
      }

      //Copying the contents to user data , by moving ahead with new blocks if multiple callbacks happen
      std::copy((char*)contents,(char*)contents+newLength,userdata->begin()+oldLength);
      return size*nmemb;
    }


/*Description - Function to invoke libcurl function for fetching the HTML content with the url
 * Input - url(the URL to whose title needs to be fetched)
 * Output - title of string, the size of string is zero if the CURL fetch is not successfull
*/
string ParseStringAPI::getCURLOp(string url){
  CURL *curl;
  CURLcode code(CURLE_FAILED_INIT);
  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  string title="",message;
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); //only for https
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); //only for https
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, handle); // Setting Callback
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &message); // Setting user data buffer
    code = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
  }

  curl_global_cleanup();
  if(message.length() != 0){//Finding the Title tag
    size_t pos_title_beg =   message.find("<title>");
    size_t pos_title_end =   message.find("</title>");
    title = message.substr(pos_title_beg+7, pos_title_end -(pos_title_beg+7) );
  }
  return title;
}

/*Description - Main function to perform string parsing
 * Input - Chat string
 * Output - Void
*/
void ParseStringAPI::ParseInput(string input){

  //Logic to find the mentions. this is done in O(n)
  size_t pos = input.find("@", 0);
  while(pos != string::npos)
  {
    stringstream mention;
    ++pos;
    while(pos < input.length()){
      if(isspace(input[pos])){ //Check to see is non word char is encountered.
                                            //Definition for non word char followed from here- http://www.cplusplus.com/reference/cctype/isspace/
        break;
      }

      mention << input[pos];
      ++pos;
    }
    m_parsedJSON["mentions"].append(mention.str());
    if(pos ==  input.length())
      break;
    else
      pos = input.find("@",pos+1);
  }

  //Logic to find the Emoticons. this is done in O(n)
  pos = input.find("(", 0);
  while(pos != string::npos)
  {
    stringstream emoticon;
    ++pos;
    int ini_pos = pos;
    bool found =false;
    while(pos <  input.length() && (pos-ini_pos) < 15){//Check for emoticon size with 15 chars

      if(input[pos] == ')' ){
        found =true;
        break;
      }else  if(isalnum(input[pos]) == 0){ //Check for if char is not alphanumeric
        break;
      }

      emoticon << input[pos];
      ++pos;

    }
    if(pos ==  input.length())
      break;
    else{
     if(found)
        m_parsedJSON["emoticons"].append(emoticon.str());

      pos = input.find("(",pos+1);
    }
  }

  //Logic to find the http:// Links. this is done in O(n)
  pos = input.find("http://", 0);
  while(pos != string::npos)
  {
    stringstream url;
    while( pos <  input.length()){
      if(isspace(input[pos]))
        break;

      url << input[pos];
      ++pos;
    }
    string title = getCURLOp(url.str());
    if(title.length()!=0){
      Value link;
      link["url"] = url.str();
      link["title"] = title;
      m_parsedJSON["links"].append(link);
    }
    if(pos ==  input.length()-1)
      break;
    else
      pos = input.find("http://",pos+1);
  }


  //Logic to find the https:// Links. this is done in O(n)
  pos = input.find("https://", 0);
  while(pos != string::npos)
  {
    stringstream url;
    while( pos <  input.length()){
      if(isspace(input[pos]))
        break;
      url << input[pos];
      ++pos;
    }
    string title = getCURLOp(url.str());
    if(title.length()!=0){
      Value link;
      link["url"] = url.str();
      link["title"] = title;
      m_parsedJSON["links"].append(link);
    }
    if(pos ==  input.length()-1)
      break;
    else
      pos = input.find("https://",pos+1);
  }

  //Clearing out any fields (mentions,links or emoticons) if its empty
  if(m_parsedJSON["links"].size() == 0)
    m_parsedJSON.removeMember("links");
  if(m_parsedJSON["mentions"].size() == 0)
    m_parsedJSON.removeMember("mentions");
  if(m_parsedJSON["emoticons"].size() == 0)
    m_parsedJSON.removeMember("emoticons");

}

/*Description - REST API call that returns JSON Value (Please refer to http://www.json.org for a detailed description of Value).
 * Input - chat message (string)
 * Output - JSON Value (Value) - JSON object containing information about its contents. This function parses the "mentions", "emoticons" and
 *              links from the input
*/
Value ParseStringAPI::GetJSONMessage(string input){
  m_parsedJSON.clear();
  ParseInput(input);
  return m_parsedJSON;
}


// Main function to perform Unit Tests
int main() {

	ParseStringAPI s;
	Value UT1 = s.GetJSONMessage("@bob @john Good morning! (megusta) (coffee) such a cool feature; https://twitter.com/jdorfman/status/430511497475670016 http://www.nbcolympics.com");
	cout<<UT1<<endl;
	Value UT2 = s.GetJSONMessage("@bob @john");
	cout<<UT2<<endl;
	Value UT3 = s.GetJSONMessage("@bob@john");
	cout<<UT3<<endl;
	Value UT4 = s.GetJSONMessage("(megusta) (coffee)");
	cout<<UT4<<endl;
	Value UT5 = s.GetJSONMessage("(megusta)@bob (coffee)");
	cout<<UT5<<endl;
	Value UT6 = s.GetJSONMessage("http://https://twitter.com/jdorfman/status/430511497475670016 http://www.nbcolympics.com");
	cout<<UT6<<endl;
	Value UT7 = s.GetJSONMessage("(megusta)@bob (coffee)http://https://twitter.com/jdorfman/status/430511497475670016 http://www.nbcolympics.com");
	cout<<UT7<<endl;
	Value UT8 = s.GetJSONMessage("(megusta@dummyname2 )");
	cout<<UT8<<endl;
	Value UT9 = s.GetJSONMessage("http://test.com/(megusta)/emoticon ");
	cout<<UT9<<endl;
	Value UT10 = s.GetJSONMessage("((nested))"); // Voilates alphanumeric test after the occurence of '('
	cout<<UT10<<endl;



	return 0;
}

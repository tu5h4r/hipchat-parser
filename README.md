Introduction
------------

This is the code for the REST API implemented in C++ for the Atlassian problem for chat string parser. The code uses one opensource library called [JSONCPP][jsoncpp] which is a C++ library that allows manipulating JSON values. As mentioned in its repo the recommended approach to integrating JsonCpp in the project is to include the [amalgamated source](#generating-amalgamated-source-and-header) (a single `.cpp` file and two `.h` files) in the project, and compile and build as any other source file.  The methodology to use JSON CPP over other C++ libraries is based on time complexities for parsing and serialization
*  The methodology results can be found here - https://blog.thousandeyes.com/efficiency-comparison-c-json-libraries
[jsoncpp]: https://github.com/open-source-parsers/jsoncpp/edit/master/

Another library that is used is called libcurl to fetch the HTML contents based on URL passed
 * An opensource/free and robust library with API details found at https://curl.haxx.se/libcurl/c/


The source include the main() function whose primary function is for Unit Tests.

About the code
---------------
* The code is src/HipChat.cpp
* json.cpp and json/\* are the amalgamated files of JSONCPP library as mentioned above
* Funtion ParseStringAPI::GetJSONMessage is the API call that takes the chat string and returns the JSON object. Each call to ParseStringAPI::GetJSONMessage resets the old JSON object and parses the new string afresh.
* Returned JSON object has 3 nodes as - mentions (if exists), emoticons(if exists) and link (if its a valid URL whose HTML content can be fetched by CURL)
* The time complexity of  ParseStringAPI::ParseInput is of the order of O(n)
  * The logic first goes through the string to find linearly all occurences of '@' , once found the end of [non word char][chr] marks the end of one mentions, the parsing continues till end untill all mentions are fetched
  [chr]: http://www.cplusplus.com/reference/cctype/isspace/
  
   *  It then goes through all occurences of '(', once found it checks if before the occurence ')' any non-alphanumeric char is encountered or 15 characeters have been parsed. If not then we have found an emoticon other ise the search cinuout linearly forward other emoticons
   * Finally for URLs, there are 2 searches. One for 'http://' and another for 'https://'. In similar logic as above all the links as parsed through the string. On each occurence of a link, the curl resquest is made to fetch the HTML content on the page. If it yeild a valid result , it means the link is valid link and its title is fetched.
   * Finally if any of the 3 nodes in the final JSON is empty, its removed.
  
  The unit test provided in the code covers some basic cases, and few edge cases.
  
  Assumptions
  -----------
* Links are condsidered to be ending with non word chars like mentions.
* Scenarios in which within an incomplete mention/emoticon/url, another valid/invalid mention/emoticon/url exists
    e.g (megusta@dummyname2 ) -> in this scnario @dummyname2 is included as mention
* Scenarios in which within an complete mention/emoticon/url, another valid mention/emoticon/url exists
  e.g http://test.com/(megusta)/emoticon -> in this scnario Link http://test.com/(megusta)/emoticon will be a valid URL with Title 302 not found  as curl fetch will succeed and (megusta) as emoticon will succeed.
  
  Limitation
  ----------
 * Due to the O(n) complexity, the code will handlle senarios of continuous mentions and will take the the outermost mention  as valid 
  e.g @bob@marley -> yeilds 1 mentiosn a bob@marley

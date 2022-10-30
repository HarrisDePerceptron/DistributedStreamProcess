#pragma once

#include <iostream>
#include <string>
#include <vector>


#include <libxml2/libxml/HTMLparser.h>
#include <libxml2/libxml/xpath.h>
#include <libxml2/libxml/xpathInternals.h>
#include <map>


#include <sstream>

struct SimpleNode {
	std::string name;
	std::string content;
	std::map<std::string, std::string>  attributes;

	
};

class SimpleHtmlParser
{
	std::string original{""};
	htmlParserCtxtPtr ctx {nullptr};

	xmlNodePtr root {nullptr};


	void parse()
	{

		ctx = htmlCreateMemoryParserCtxt(original.c_str(), original.size());
		auto xmlhhtml = htmlParseDocument(ctx);
		if (xmlhhtml < 0)
		{
			std::ostringstream iss;
			iss << xmlhhtml;
			auto erroCode = iss.str();
			std::cout << "Warning: there maybe some error parsing the html. Error code: " + erroCode << std::endl;
		}

		if (ctx->myDoc == nullptr)
		{
			std::cout << "myDoc is null" << std::endl;
			throw std::runtime_error{"my doc is null"};
		}

		auto xmldoc = ctx->myDoc;
		root = xmlDocGetRootElement(xmldoc);
		if (!root)
		{
			throw std::runtime_error{"root is null"};
		}

		std::string parentName = (const char *)root->name;

		
	}

public:
	SimpleHtmlParser() = delete;
	SimpleHtmlParser(const std::string &htmlOriginal) : original{htmlOriginal}
	{
		parse();
	}

	std::vector<SimpleNode> getNodeByXpath(std::string xpath){

		std::vector<SimpleNode> simpleNodes;

		auto xpathCtx = xmlXPathNewContext(ctx->myDoc);

		if (xpathCtx == nullptr)
		{
			throw std::runtime_error{"xpath context unable to init"};
		}

		auto xpathExpression = xmlXPathEvalExpression((const xmlChar *)xpath.c_str(), xpathCtx);
			

		if (xpathExpression == nullptr)
		{
			throw std::runtime_error{"xpath expression error"};
		}

		if (xpathExpression->nodesetval == nullptr){
			return simpleNodes;
		}

		const auto numNodes = xpathExpression->nodesetval->nodeNr;
	

		for (int i = 0; i < numNodes; i++)
		{
			auto node = xpathExpression->nodesetval->nodeTab[i];
			if (node->ns)
			{
				std::cout << "namespace: " << node->ns->href << std::endl;
			}

			auto nodeContentXml = xmlNodeGetContent(node);
	
			std::string nodeContent = "";
			std::string nodeName = "";
			
			if(nodeContentXml!=nullptr){
				nodeContent = (const char *)xmlNodeGetContent(node);
			}
			
			if(node->name!=nullptr){
				nodeName = (const char *)node->name;

			}			

			SimpleNode sn;
			sn.name = nodeName;
			sn.content = nodeContent;

			
			for(auto & prop = node->properties;prop!=nullptr;prop=prop->next){
				std::string propName = (const char *)prop->name;
				std::string propValue =  (const char *)xmlGetProp(node, (const xmlChar *)propName.c_str());
				sn.attributes.insert({propName, propValue});
			}

			
			
			simpleNodes.push_back(sn);
					
		}
		xmlXPathFreeObject(xpathExpression);
		xmlXPathFreeContext(xpathCtx);

		return simpleNodes;


	}

	virtual ~SimpleHtmlParser()
	{

		htmlFreeParserCtxt(ctx);
	}
};


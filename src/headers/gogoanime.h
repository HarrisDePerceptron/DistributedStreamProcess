#pragma once

#include <iostream>
#include <vector>
#include <thread>

#include <cpr/cpr.h>

#include <fstream>

#include "simplehtmlparser.h"
#include <future>


struct Anime
{
	std::string name;
	std::vector<std::string> genre;
	std::string summary;
	std::string released;
	std::vector<std::string> episodes;
	std::string image;
	std::string status;
	std::string season;

	void print() const
	{
		std::cout << "Name: " << name
				  << "\nGenre: " << [&]
		{std::string genreS; for (const auto & e:  genre)genreS += e + ","; return genreS; }()
				  << "\nSummary: " << summary
				  << "\nReleased: " << released
				  << "\nStatus: " << status
				  << "\nSeason: " << season
				  << "\nImage: " << image
				  << std::endl;
	}
};

std::string extractDetail(std::string baseUrl, std::string link)
{

	cpr::Response r = cpr::Get(cpr::Url{link});
	std::string out = r.text.c_str();

	SimpleHtmlParser sp{out};
	std::string xpathDetailLink = "//div[@class='anime-info']/a";

	auto searchResponse = sp.getNodeByXpath(xpathDetailLink);
	for (const auto &n : searchResponse)
	{
		for (const auto &[key, value] : n.attributes)
		{

			if (key != "href")
			{
				continue;
			}

			std::string detailLink = baseUrl + value;
			return detailLink;
		}
	}

	return "";
}

Anime extractDetailPage(std::string baseUrl, std::string link)
{
	Anime anim;
	cpr::Response r = cpr::Get(cpr::Url{link});
	std::string out = r.text.c_str();

	SimpleHtmlParser sp{out};

	std::string xpathDetailLink = "//div[@class='anime_info_body_bg']/h1";
	auto titleResponse = sp.getNodeByXpath(xpathDetailLink);
	for (const auto &n : titleResponse)
	{
		std::string title = n.content;
		anim.name = title;
		break;
	}

	std::string xpathStatus = "//div[@class='anime_info_body_bg']/p[@class='type'][5]/a";
	auto statusResponse = sp.getNodeByXpath(xpathStatus);
	for (const auto &n : statusResponse)
	{
		std::string status = n.content;
		anim.status = status;
		break;
	}

	std::string xpathReleased = "//div[@class='anime_info_body_bg']/p[@class='type'][4]";
	auto releasedResponse = sp.getNodeByXpath(xpathReleased);
	for (const auto &n : releasedResponse)
	{
		std::string released = n.content;
		anim.released = released;
		break;
	}

	std::string xpathGenre = "//div[@class='anime_info_body_bg']/p[@class='type'][3]/a";
	auto genreResponse = sp.getNodeByXpath(xpathGenre);
	for (const auto &n : genreResponse)
	{
		std::string genre = n.content;
		anim.genre.push_back(genre);
	}

	std::string xpathType = "//div[@class='anime_info_body_bg']/p[@class='type'][1]/a";
	auto typeResponse = sp.getNodeByXpath(xpathType);
	for (const auto &n : typeResponse)
	{
		std::string type = n.content;
		anim.season = type;
	}

	std::string xpathSummary = "//div[@class='anime_info_body_bg']/p[@class='type'][2]";
	auto summaryResponse = sp.getNodeByXpath(xpathSummary);
	for (const auto &n : summaryResponse)
	{
		std::string summary = n.content;
		anim.summary = summary;
	}

	return anim;
}



std::vector<std::string> getLatest(std::string base_url)
{
	cpr::Response r = cpr::Get(cpr::Url{base_url});
	std::string html = r.text.c_str();
	SimpleHtmlParser htmlParser{html};

	std::string xpathSelector = "//div[@class='last_episodes loaddub']/ul/li[*]/p[1]/a";

	auto resNodes = htmlParser.getNodeByXpath(xpathSelector);

	std::vector<std::string> linksToFollow;

	for (const auto &e : resNodes)
	{

		std::cout << "Name: " << e.name << ": " << e.content << std::endl;

		for (const auto &[key, value] : e.attributes)
		{
			if (key != "href")
			{
				continue;
			}

			std::string link = value;
			link = base_url + link;
			linksToFollow.push_back(link);
		}
	}
	return linksToFollow;
}

std::vector<std::string> getLatestPaged(const std::string &base_url, int page)
{

	const std::string url = "https://ajax.gogo-load.com/ajax/page-recent-release.html?page=" + std::to_string(page) + "&type=1";

	cpr::Response r = cpr::Get(cpr::Url{url});
	std::string html = r.text.c_str();
	SimpleHtmlParser htmlParser{html};

	std::string xpathSelector = "//div[@class='last_episodes loaddub']/ul/li[*]/p[1]/a";

	auto resNodes = htmlParser.getNodeByXpath(xpathSelector);

	std::vector<std::string> linksToFollow;

	for (const auto &e : resNodes)
	{

		for (const auto &[key, value] : e.attributes)
		{
			if (key != "href")
			{
				continue;
			}

			std::string link = value;
			link = base_url + link;
			linksToFollow.push_back(link);
		}
	}
	return linksToFollow;
}

std::vector<Anime> getAnimePaged(std::string base_url, int page)
{

	auto linksToFollow = getLatestPaged(base_url, page);

	std::vector<std::future<std::string>> detailFutures;
	for (const auto &e : linksToFollow)
	{
		auto f = std::async(std::launch::async, [&]()
							{ return extractDetail(base_url, e); });
		detailFutures.push_back(std::move(f));
	}

	std::vector<std::string> detailLinks;
	for (auto &e : detailFutures)
	{
		auto detailLink = e.get();
		detailLinks.push_back(detailLink);
	}

	std::vector<std::future<Anime>> animeFutures;
	for (const auto &e : detailLinks)
	{
		auto f = std::async(std::launch::async, [&]()
							{ return extractDetailPage(base_url, e); });
		animeFutures.push_back(std::move(f));
	}

	std::vector<Anime> animes;
	for (auto &e : animeFutures)
	{
		auto animeRes = e.get();
		animes.push_back(animeRes);
	}

	return animes;
}


std::vector<Anime> extractAnimePages(const std::string & base_url, const int totalPages){

	std::vector<Anime> animes;

	
	std::vector<std::future<std::vector<Anime>>> futs;

	for(int i =1; i<totalPages+1; i++){
		auto animeResFut = std::async(std::launch::async, [&](){
			auto animeRes = getAnimePaged(base_url, i);
			return animeRes;
		});
		
		futs.push_back(std::move(animeResFut));
	}

	for(auto & e: futs){
		auto res = e.get();
		animes.insert(animes.end(), res.begin(), res.end());
	}

	

	return animes;

	
}
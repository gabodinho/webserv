/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Router.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: okrahl <okrahl@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/16 17:44:54 by okrahl            #+#    #+#             */
/*   Updated: 2024/10/30 18:22:53 by okrahl           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Router.hpp"
#include "Helper.hpp"
#include <sstream>
#include <ctime>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cerrno>
#include <vector>
#include <dirent.h>

Router::Router(ServerConfig& config) : _serverConfig(config) {
	// Initialization code
}

Router::~Router() {
	// Cleanup code
}

void Router::addRoute(const std::string& path, RouteHandler handler, const std::string& uploadDir) {
	routes[path] = handler;
	if (!uploadDir.empty()) {
		uploadDirs[path] = uploadDir;
	}
}

void Router::handleRequest(const HttpRequest& request, HttpResponse& response) {
	std::string path = request.getUrl();
	size_t queryPos = path.find('?');
	if (queryPos != std::string::npos) {
		path = path.substr(0, queryPos);
	}

	std::string method = request.getMethod();

	// Print all stored locations
	const std::map<std::string, Location>& locations = _serverConfig.getLocations();
	std::cout << "Stored Locations:" << std::endl;
	for (std::map<std::string, Location>::const_iterator it = locations.begin(); it != locations.end(); ++it) {
		std::cout << "Path: " << it->first << "\n" << it->second << std::endl;
	}

	// Show which location is being searched for
	std::cout << "Searching for Location: " << path << std::endl;

	// Find the matching location
	try {
		Location location = _serverConfig.findLocation(path);
		std::cout << "Found Location: " << path << std::endl;

		// Check if the method is allowed
		if (!location.isMethodAllowed(method)) {
			setErrorResponse(response, 405);
			return;
		}

		// Execute the route if the method is allowed
		std::map<std::string, RouteHandler>::const_iterator it = routes.find(path);
		if (it != routes.end()) {
			std::cout << "Found Route: " << path << std::endl;
			RouteHandler handler = it->second;
			(this->*handler)(request, response); // Call the member function
		} else {
			std::cout << "Route not found: " << path << std::endl;
			setErrorResponse(response, 404);
		}
	} catch (const ServerConfig::LocationNotFound& e) {
		std::cout << "Location not found: " << path << std::endl;
		setErrorResponse(response, 404);
	}
}

void Router::setErrorResponse(HttpResponse& response, int errorCode) {
	const std::map<int, std::string>& errorPages = _serverConfig.getErrorPages();

	// Print all error pages
	std::cout << "Available Error Pages:" << std::endl;
	for (std::map<int, std::string>::const_iterator it = errorPages.begin(); it != errorPages.end(); ++it) {
		std::cout << "Error Code: " << it->first << ", Path: " << it->second << std::endl;
	}

	// Print the error code being searched for
	std::cout << "Searching for Error Page with code: " << errorCode << std::endl;

	std::map<int, std::string>::const_iterator errorPageIt = errorPages.find(errorCode);

	std::string errorPageContent;
	if (errorPageIt != errorPages.end()) {
		std::cout << "Found Error Page Path: " << errorPageIt->second << std::endl;
		errorPageContent = _serverConfig.readFile(errorPageIt->second);
	} else {
		std::cout << "Error Page Not Found for code: " << errorCode << std::endl;
		errorPageContent = "<html><body><h1>Error Page Not Found</h1></body></html>";
	}

	response.setStatusCode(errorCode);
	response.setBody(errorPageContent);
	response.setHeader("Content-Type", "text/html");
}

void Router::handleHomeRoute(const HttpRequest& req, HttpResponse& res) {
	if (req.getMethod() == "GET") {
		std::string content = readFile("HTMLFiles/welcome.html");
		res.setStatusCode(200);
		res.setBody(content);
		res.setHeader("Content-Type", "text/html");
	} else {
		setErrorResponse(res, 405);
	}
}

void Router::handleFormRoute(const HttpRequest& req, HttpResponse& res) {
	if (req.getMethod() == "GET") {
		std::string content = readFile("HTMLFiles/form.html");
		res.setStatusCode(200);
		res.setBody(content);
		res.setHeader("Content-Type", "text/html");
	} else {
		setErrorResponse(res, 405);
	}
}

void Router::handleUploadRoute(const HttpRequest& req, HttpResponse& res) {
	std::cout << "handleUploadRoute called" << std::endl;
	std::cout << "Request URL: " << req.getUrl() << std::endl;
	std::cout << "Request Method: " << req.getMethod() << std::endl;

	// Retrieve the upload directory from the configuration
	try {
		Location uploadLocation = _serverConfig.findLocation("/upload");
		std::string uploadDir = uploadLocation.getRoot();

		if (req.getMethod() == "GET") {
			std::string content = readFile("HTMLFiles/upload.html");
			res.setStatusCode(200);
			res.setBody(content);
			res.setHeader("Content-Type", "text/html");
			std::cout << "Response: 200 OK (Upload GET)" << std::endl;
		} else if (req.getMethod() == "POST") {
			std::cout << "Processing POST request" << std::endl;

			const std::vector<std::string>& filenames = req.getFilenames();
			if (filenames.empty()) {
				std::cout << "No files uploaded" << std::endl;
				setErrorResponse(res, 400);
				return;
			}

			saveUploadedFiles(req, uploadDir);

			// Redirect to the success page
			res.setStatusCode(302);
			res.setHeader("Location", "/uploadSuccessful");
			std::cout << "Response: 302 Redirect to /uploadSuccessful" << std::endl;
		} else if (req.getMethod() == "DELETE") {
			std::cout << "Processing DELETE request" << std::endl;
			std::string filename = extractFilenameFromUrl(req.getUrl());
			std::cout << "Extracted filename: " << filename << std::endl;

			if (filename.empty()) {
				std::cout << "Filename not found in DELETE request" << std::endl;
				setErrorResponse(res, 400);
				return;
			}

			std::string filePath = uploadDir + "/" + filename;
			std::cout << "Attempting to delete file: " << filePath << std::endl;

			if (remove(filePath.c_str()) == 0) {
				std::cout << "File deleted successfully" << std::endl;
				uploadedFiles.erase(std::remove(uploadedFiles.begin(), uploadedFiles.end(), filename), uploadedFiles.end());
				res.setStatusCode(200);
				res.setBody("File Deleted Successfully");
				std::cout << "Response: 200 OK (File Deleted)" << std::endl;
			} else {
				std::cout << "Error deleting file: " << strerror(errno) << std::endl;
				setErrorResponse(res, 404);
			}
			res.setHeader("Content-Type", "text/plain");
		} else {
			std::cout << "Method not allowed: " << req.getMethod() << std::endl;
			setErrorResponse(res, 405);
		}
	} catch (const ServerConfig::LocationNotFound& e) {
		std::cout << "Upload location not found." << std::endl;
		setErrorResponse(res, 404);
	}
}

void Router::handleUploadSuccessRoute(const HttpRequest& req, HttpResponse& res) {
	if (req.getMethod() == "GET") {
		try {
			Location uploadLocation = _serverConfig.findLocation("/uploadSuccessful");
			std::string successFilePath = uploadLocation.getRoot() + "/uploadSuccessful.html";

			std::cout << "Accessing file: " << successFilePath << std::endl;

			std::string successContent = readFile(successFilePath);

			// Get the list of files
			Location uploadDirLocation = _serverConfig.findLocation("/upload");
			std::string uploadDir = uploadDirLocation.getRoot();
			std::vector<std::string> files = getFilesInDirectory(uploadDir);

			// Convert file list to JSON
			std::ostringstream json;
			json << "[";
			for (size_t i = 0; i < files.size(); ++i) {
				if (i > 0) json << ", ";
				json << "\"" << files[i] << "\"";
			}
			json << "]";

			// Inject the file list into the HTML
			std::string script = "<script>const fileList = " + json.str() + ";</script>";
			size_t pos = successContent.find("</head>");
			if (pos != std::string::npos) {
				successContent.insert(pos, script);
			}

			res.setStatusCode(200);
			res.setBody(successContent);
			res.setHeader("Content-Type", "text/html");
		} catch (const ServerConfig::LocationNotFound& e) {
			setErrorResponse(res, 404);
		} catch (const std::exception& e) {
			std::cout << "Error reading file: " << e.what() << std::endl;
			setErrorResponse(res, 404);
		}
	} else {
		setErrorResponse(res, 405);
	}
}

std::vector<std::string> Router::getFilesInDirectory(const std::string& directory) {
	std::vector<std::string> files;
	DIR* dir = opendir(directory.c_str());
	if (dir) {
		struct dirent* entry;
		while ((entry = readdir(dir)) != NULL) { // Use NULL
			if (entry->d_type == DT_REG) {
				files.push_back(entry->d_name);
			}
		}
		closedir(dir);
	}
	return files;
}

void Router::saveUploadedFiles(const HttpRequest& req, const std::string& uploadDir) {
	const std::vector<std::string>& filenames = req.getFilenames();
	const std::string& body = req.getBody();
	size_t pos = 0;

	std::cout << "Full body received (first 1000 chars): " << body.substr(0, 1000) << std::endl;
	if (body.size() > 1000) {
		std::cout << "  (truncated, total size: " << body.size() << " bytes)\n";
	}

	ensureDirectoryExists(uploadDir);

	for (size_t i = 0; i < filenames.size(); ++i) {
		size_t start = body.find("\r\n\r\n", pos) + 4;
		if (start == std::string::npos) {
			std::cerr << "Failed to find the start of the file content for file: " << filenames[i] << std::endl;
			continue;
		}

		size_t end = body.find("\r\n--", start);
		if (end == std::string::npos) {
			std::cerr << "Failed to find the end of the file content for file: " << filenames[i] << std::endl;
			continue;
		}

		std::string fileContent = body.substr(start, end - start);
		pos = end + 4;

		if (fileContent.size() >= 2 && fileContent.compare(fileContent.size() - 2, 2, "\r\n") == 0) {
			fileContent.erase(fileContent.size() - 2);
		}

		std::string savedFilename = uploadDir + "/" + filenames[i].substr(filenames[i].find_last_of("\\/") + 1);
		std::ofstream outFile(savedFilename.c_str(), std::ios::binary);
		if (outFile.is_open()) {
			outFile.write(fileContent.c_str(), fileContent.size());
			outFile.close();
			std::cout << "File saved successfully: " << savedFilename << std::endl;
			uploadedFiles.push_back(savedFilename);
		} else {
			std::cerr << "Failed to open file for writing: " << savedFilename << std::endl;
		}
	}
}

void Router::initializeRoutes() {
	addRoute("/", &Router::handleHomeRoute);
	addRoute("/upload", &Router::handleUploadRoute);
	addRoute("/uploadSuccessful", &Router::handleUploadSuccessRoute);
	addRoute("/form", &Router::handleFormRoute);
}
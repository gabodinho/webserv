/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Router.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: okrahl <okrahl@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/08 14:54:49 by okrahl            #+#    #+#             */
/*   Updated: 2024/10/16 15:50:07 by okrahl           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Router.hpp"
#include "Helper.hpp"

Router::Router() {
	// Konstruktor-Implementierung (falls erforderlich)
}

Router::~Router() {
	// Destruktor-Implementierung (falls erforderlich)
}

void Router::addRoute(const std::string& path, RouteHandler handler) {
	routes[path] = handler;
}

void Router::handleRequest(const HttpRequest& request, HttpResponse& response) {
	std::cout << "Request URL: " << request.getUrl() << std::endl;
	std::cout << "Request Method: " << request.getMethod() << std::endl;

	// Extract Path without Query-Parameter
	std::string path = request.getUrl();
	size_t queryPos = path.find('?');
	if (queryPos != std::string::npos) {
		path = path.substr(0, queryPos);
	}

	std::map<std::string, RouteHandler>::const_iterator it = routes.find(path);
	if (it != routes.end()) {
		RouteHandler handler = it->second;
		(this->*handler)(request, response); // Call the member function
	} else {
		response.setStatusCode(404);
		response.setBody("<html><body><h1>404 Not Found</h1></body></html>");
		response.setHeader("Content-Type", "text/html");
	}
}

void Router::handleHomeRoute(const HttpRequest& req, HttpResponse& res) {
	if (req.getMethod() == "GET") {
		std::string content = readFile("HTMLFiles/welcome.html");
		res.setStatusCode(200);
		res.setBody(content);
		res.setHeader("Content-Type", "text/html");
	} else {
		res.setStatusCode(405);
		res.setBody("<html><body><h1>405 Method Not Allowed</h1></body></html>");
		res.setHeader("Content-Type", "text/html");
	}
}

void Router::handleFormRoute(const HttpRequest& req, HttpResponse& res) {
	if (req.getMethod() == "GET") {
		std::string content = readFile("HTMLFiles/form.html");
		res.setStatusCode(200);
		res.setBody(content);
		res.setHeader("Content-Type", "text/html");
	} else {
		res.setStatusCode(405);
		res.setBody("<html><body><h1>405 Method Not Allowed</h1></body></html>");
		res.setHeader("Content-Type", "text/html");
	}
}

void Router::handleUploadRoute(const HttpRequest& req, HttpResponse& res) {
	std::cout << "handleUploadRoute called" << std::endl;
	std::cout << "Request URL: " << req.getUrl() << std::endl;
	std::cout << "Request Method: " << req.getMethod() << std::endl;

	if (req.getMethod() == "GET") {
		std::string content = readFile("HTMLFiles/upload.html");
		res.setStatusCode(200);
		res.setBody(content);
		res.setHeader("Content-Type", "text/html");
	} else if (req.getMethod() == "POST") {
		std::cout << "Processing POST request" << std::endl;

		const std::vector<std::string>& filenames = req.getFilenames();
		if (filenames.empty()) {
			std::cout << "No files uploaded" << std::endl;
			res.setStatusCode(400);
			res.setBody("Bad Request: No files uploaded");
			res.setHeader("Content-Type", "text/plain");
			return;
		}

		saveUploadedFiles(req);

		std::string successContent = readFile("HTMLFiles/uploadSuccessful.html");
		res.setStatusCode(200);
		res.setBody(successContent);
		res.setHeader("Content-Type", "text/html");
	} else if (req.getMethod() == "DELETE") {
		std::cout << "Processing DELETE request" << std::endl;
		std::string filename = extractFilenameFromUrl(req.getUrl());
		std::cout << "Extracted filename: " << filename << std::endl;

		if (filename.empty()) {
			std::cout << "Filename not found in DELETE request" << std::endl;
			res.setStatusCode(400);
			res.setBody("Bad Request: Filename not found");
			res.setHeader("Content-Type", "text/plain");
			return;
		}

		std::string filePath = "./uploads/" + filename;
		std::cout << "Attempting to delete file: " << filePath << std::endl;

		if (remove(filePath.c_str()) == 0) {
			std::cout << "File deleted successfully" << std::endl;
			uploadedFiles.erase(std::remove(uploadedFiles.begin(), uploadedFiles.end(), filename), uploadedFiles.end());
			res.setStatusCode(200);
			res.setBody("File Deleted Successfully");
		} else {
			std::cout << "Error deleting file: " << strerror(errno) << std::endl;
			res.setStatusCode(404);
			res.setBody("File Not Found");
		}
		res.setHeader("Content-Type", "text/plain");
	} else {
		std::cout << "Method not allowed: " << req.getMethod() << std::endl;
		res.setStatusCode(405);
		res.setBody("<html><body><h1>405 Method Not Allowed</h1></body></html>");
		res.setHeader("Content-Type", "text/html");
	}
}

void Router::saveUploadedFiles(const HttpRequest& req) {
	const std::vector<std::string>& filenames = req.getFilenames();
	const std::string& body = req.getBody();
	size_t pos = 0;

	// Create the uploads directory if it doesn't exist
	ensureDirectoryExists("uploads");

	for (size_t i = 0; i < filenames.size(); ++i) {
		size_t start = body.find("\r\n\r\n", pos) + 4;
		size_t end = body.find("\r\n--", start);
		std::string fileContent = body.substr(start, end - start);
		pos = end + 4;

		std::string savedFilename = "uploads/" + filenames[i].substr(filenames[i].find_last_of("\\/") + 1);
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
	addRoute("/form", &Router::handleFormRoute);
}
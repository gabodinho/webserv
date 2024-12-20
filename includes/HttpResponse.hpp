/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: okrahl <okrahl@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/02 17:16:51 by okrahl            #+#    #+#             */
/*   Updated: 2024/11/11 16:03:59 by okrahl           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>
#include "HttpRequest.hpp"

class HttpResponse
{
	public:

		HttpResponse();
		HttpResponse(const HttpRequest& request);

		void setStatusCode(int code);
		void setHeader(const std::string& key, const std::string& value);
		void setBody(const std::string& body);

		std::string toString() const;

	private:

		std::string version;
		int statusCode;
		std::string statusMessage;
		std::map<std::string, std::string> headers;
		std::string body;

		void setStatusMessage(int code);
};

#endif // HTTPRESPONSE_HPP
#ifndef HTTPSERVER_STRINGS_H
#define HTTPSERVER_STRINGS_H

namespace httpc {
namespace server {
namespace strings {
	namespace request {
		static const char* art = "/%s_";	// art request template
		static const char* robots_txt = "/robots.txt";
	}
	namespace header {
		static const char* no_cache = "Cache-Control: no-cache\r\nPragma: no-cache\r\nExpires: Mon, 02 Jun 1980 01:02:03 GMT";
		static const char* last_modified = "Last-Modified: %s";
		static const char* content_type_text_plain = "Content-Type: text/plain; charset=utf-8";
		static const char* content_type_text_html = "Content-Type: text/html; charset=utf-8";
		static const char* content_type = "Content-Type: %s";
		static const char* content_length = "Content-Length: %d";
		static const char* content_encoding_gzip = "Content-Encoding: gzip";
		static const char* connection_close = "Connection: close";
		static const char* auth_basic = "WWW-Authenticate: Basic realm=\"Authorization\"";
	}
	namespace reply {
		static const char* c200 = "HTTP/1.0 200 OK";
		static const char* c401 = "HTTP/1.0 401 Authorization Required";
		static const char* c403 = "HTTP/1.0 403 Forbidden";
		static const char* c404 = "HTTP/1.0 404 Not Found";
	}
	namespace title {
		static const char* c403 = "403 Forbidden";
		static const char* c404 = "404 Not Found";
		static const char* template_config_error = "Template error";
	}
	namespace body {
		static const char* c403 = "<h1>403 Forbidden</h1><p>You don't have permission to access this document on this server.</p>";
		static const char* c404 = "<h1>404 Not Found</h1><p>The requested url %s was not found on the server.</p>";
		static const char* template_config_error = "<p>Error loading template configuration file:<br>foo_httpcontrol_data\\%s\\config</p>\
<p>Most likely it happened because the directory you are trying to open doesn't contain template, or template files there are incomplete.</p>\
<p><font color = \"red\">Check foobar2000 console (View/Console) for error details.</font></p>\
<p><a href=\"/\">Show installed templates</a></p>";
		static const char* robots_txt = "User-agent: *\r\nDisallow: /";
	}
}
}
}

#endif
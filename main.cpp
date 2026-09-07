#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <atomic>

static std::atomic<std::size_t> view_counter{};

constexpr std::string_view kPageHeader = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>James Lim</title>
</head>
<body>
<h1>James Lim</h1>
<p>I write C++</p>
<p>
<a href="https://github.com/lim-james">GitHub</a> &middot;
<a href="https://www.linkedin.com/in/jameslimbj">LinkedIn</a>
</p>
<p>)HTML";

constexpr std::string_view kPageTail = R"HTML( views since server restarted.</p>
</body>
</html>)HTML";

namespace {

boost::beast::http::response<boost::beast::http::string_body> make_response(
  const boost::beast::http::request<boost::beast::http::string_body>& request
) {
  boost::beast::http::response<boost::beast::http::string_body> response;
  response.version(request.version());
  response.keep_alive(request.keep_alive());
  response.set(boost::beast::http::field::server, "beast");

  const bool is_head = request.method() == boost::beast::http::verb::head;

  if (request.method() != boost::beast::http::verb::get && !is_head) {
    response.result(boost::beast::http::status::method_not_allowed);
    response.set(boost::beast::http::field::allow, "GET, HEAD");
    response.set(boost::beast::http::field::content_type, "text/plain");
    response.body() = "method not allowed\n";
  } else if (request.target() != "/" && request.target() != "/index.html") {
    response.result(boost::beast::http::status::not_found);
    response.set(boost::beast::http::field::content_type, "text/plain");
    response.body() = "not found\n";
  } else {
    const auto count = view_counter.fetch_add(1, std::memory_order_relaxed) + 1;
    std::string count_str = std::to_string(count); 

    std::string page{};
    page.reserve(kPageHeader.length() + count_str.length() + kPageTail.length());
    page.append(kPageHeader).append(count_str).append(kPageTail);

    response.result(boost::beast::http::status::ok);
    response.set(boost::beast::http::field::content_type, "text/html; charset=utf-8");
    response.body() = std::move(page);
  }

  const auto len = response.body().size();
  response.prepare_payload();
  if (is_head) {
    response.body().clear();
    response.content_length(len);
  }
  return response;
}

void handle_session(boost::asio::ip::tcp::socket socket) {
  boost::beast::error_code error_code;
  boost::beast::flat_buffer buffer;

  for (;;) {
    boost::beast::http::request<boost::beast::http::string_body> request;
    boost::beast::http::read(socket, buffer, request, error_code);
    if (error_code) break;

    auto response = make_response(request);
    const bool keep = response.keep_alive();
    boost::beast::http::write(socket, response, error_code);
    if (error_code || !keep) break;
  }

  socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, error_code);
}

} // namespace

int main(int argc, char** argv) {
  try {
    const auto addr = boost::asio::ip::make_address(argc > 1 ? argv[1] : "127.0.0.1");
    const auto port = static_cast<unsigned short>(argc > 2 ? std::stoi(argv[2]) : 8080);

    boost::asio::io_context ioc{1};
    boost::asio::ip::tcp::acceptor acceptor{ioc, {addr, port}};
    std::cerr << "listening on " << addr << ":" << port << "\n";

    for (;;) {
      boost::asio::ip::tcp::socket socket{ioc};
      acceptor.accept(socket);
      std::thread(handle_session, std::move(socket)).detach();
    }
  } catch (const std::exception& e) {
    std::cerr << "fatal: " << e.what() << "\n";
    return 1;
  }
}

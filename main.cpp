#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/vector_body.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/json/parse.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/url.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

// clang-format off
static constexpr std::string_view kApiUrl = "https://bingwallpaper.microsoft.com/api/BWC/getHPImages?screenWidth=1920&screenHeight=1080&env=live";
// clang-format on

template <class T = char>
std::vector<T> HttpGet(boost::asio::io_context& ioc,
                       boost::asio::ssl::context& ctx,
                       const boost::urls::url_view& url) {
  std::string port;
  if (!url.has_port()) {
    if (url.scheme_id() == boost::urls::scheme::http) {
      port = "80";
    } else if (url.scheme_id() == boost::urls::scheme::https) {
      port = "443";
    } else {
      throw std::runtime_error("Unknown scheme default port");
    }
  } else {
    port = url.port();
  }
  auto host = url.host();
  auto target = url.encoded_target().substr();
  boost::asio::ip::tcp::resolver resolver{ioc};
  boost::beast::ssl_stream<boost::beast::tcp_stream> stream{ioc, ctx};
  auto results = resolver.resolve(host, port);
  boost::beast::get_lowest_layer(stream).connect(results);
  stream.handshake(boost::asio::ssl::stream_base::client);
  boost::beast::http::request<boost::beast::http::empty_body> req{
      boost::beast::http::verb::get, target, 11};
  req.set(boost::beast::http::field::host, host);
  boost::beast::http::write(stream, req);
  boost::beast::flat_buffer buffer;
  boost::beast::http::response<boost::beast::http::vector_body<T>> res;
  boost::beast::http::read(stream, buffer, res);
  return std::move(res.body());
}

void SetWorkDirectory() {
  wchar_t path[MAX_PATH];
  GetTempPath(MAX_PATH, path);
  SetCurrentDirectory(path);
}

std::vector<char> DownloadImage(boost::asio::io_context& ioc,
                                boost::asio::ssl::context& ctx,
                                std::string_view url) {
  boost::urls::url_view v = url;
  auto data = HttpGet(ioc, ctx, v);
  auto jv = boost::json::parse({data.data(), data.size()});
  auto img_url = jv.at("images").as_array().at(0).at("url").as_string();
  return HttpGet(ioc, ctx, img_url);
}

std::filesystem::path SaveImage(const std::vector<char>& data) {
  auto img_path = std::filesystem::current_path();
  if (!std::filesystem::exists(img_path)) {
    std::filesystem::create_directories(img_path);
  }
  img_path.append("BingWallpaper.jpg");
  std::ofstream file(img_path, std::ios_base::out | std::ios_base::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Cannot open file: " + img_path.string());
  }
  file.write(data.data(), data.size());
  file.close();
  return img_path;
}

void SetDesktopWallpaper(const std::filesystem::path& img_path) {
  bool status = SystemParametersInfo(
      SPI_SETDESKWALLPAPER, 0, (PVOID)img_path.c_str(), SPIF_UPDATEINIFILE);
  if (!status) {
    throw std::runtime_error("Set desktop wallpaper failed: " +
                             std::to_string(GetLastError()));
  }
}

int main(int argc, char* argv[]) {
  boost::asio::io_context ioc{1};
  boost::asio::ssl::context ctx(boost::asio::ssl::context::tlsv12_client);
  ctx.set_verify_mode(boost::asio::ssl::verify_none);
  try {
    SetWorkDirectory();
    std::cout << "Download image..." << std::endl;
    auto img_data = DownloadImage(ioc, ctx, kApiUrl);
    std::cout << "Save image..." << std::endl;
    auto img_path = SaveImage(img_data);
    std::cout << "Set desktop wallpaper..." << std::endl;
    SetDesktopWallpaper(img_path);
    std::cout << "Completed" << std::endl;
  } catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
    system("pause");
  }
}

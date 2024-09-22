#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/vector_body.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/json/parse.hpp>
#include <boost/url/url_view.hpp>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <vector>

namespace asio = boost::asio;
namespace urls = boost::urls;
namespace beast = boost::beast;
namespace json = boost::json;

// clang-format off
static constexpr std::string_view kApiUrlFormat = "https://global.bing.com/HPImageArchive.aspx?format=js&idx=0&n=1&uhd=1&uhdwidth={}&uhdheight={}&setmkt={}&setlang={}";
static constexpr std::string_view kImgUrlFormat = "https://cn.bing.com{}";
// clang-format on

template <class T = char>
std::vector<T> HttpGet(asio::io_context& ioc, asio::ssl::context& ctx,
                       const urls::url_view& url) {
  std::string port = "443";
  if (url.has_port()) {
    port = url.port();
  }
  auto host = url.host();
  auto target = url.encoded_target().substr();
  asio::ip::tcp::resolver resolver{ioc};
  beast::ssl_stream<beast::tcp_stream> stream{ioc, ctx};
  auto results = resolver.resolve(host, port);
  beast::get_lowest_layer(stream).connect(results);
  stream.handshake(asio::ssl::stream_base::client);
  beast::http::request<beast::http::empty_body> req{beast::http::verb::get,
                                                    target, 11};
  req.set(beast::http::field::host, host);
  beast::http::write(stream, req);
  beast::flat_buffer buffer;
  beast::http::response<beast::http::vector_body<T>> res;
  beast::http::read(stream, buffer, res);
  return std::move(res.body());
}

struct ScreenResolution {
  std::int32_t width = 0;
  std::int32_t height = 0;
};

ScreenResolution GetScreenResolution() noexcept {
  std::int32_t width = GetSystemMetrics(SM_CXSCREEN);
  std::int32_t height = GetSystemMetrics(SM_CYSCREEN);
  return {.width = width, .height = height};
}

std::vector<char> DownloadImage(asio::io_context& ioc, asio::ssl::context& ctx,
                                ScreenResolution resolution,
                                std::string_view lcid) {
  auto url = std::format(kApiUrlFormat, resolution.width, resolution.height,
                         lcid, lcid);
  std::cout << "GET " << url << std::endl;
  auto js = HttpGet(ioc, ctx, url);
  auto jv = json::parse({js.data(), js.size()});
  auto img_path = jv.at("images").as_array().at(0).at("url").as_string();
  auto img_url = std::format(kImgUrlFormat, img_path.c_str());
  std::cout << "GET " << img_url << std::endl;
  return HttpGet(ioc, ctx, img_url);
}

std::filesystem::path SaveImage(const std::vector<char>& data) {
  auto img_path = std::filesystem::current_path();
  if (!std::filesystem::exists(img_path)) {
    std::filesystem::create_directories(img_path);
  }
  img_path.append("BingWallpaper.jpg");
  std::cout << "SAVE " << img_path << std::endl;
  std::ofstream file(img_path, std::ios_base::out | std::ios_base::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Cannot open file: " + img_path.string());
  }
  file.write(data.data(), data.size());
  file.close();
  return img_path;
}

void SetDesktopWallpaper(const std::filesystem::path& img_path) {
  std::cout << "SET " << img_path << std::endl;
  bool status = SystemParametersInfo(
      SPI_SETDESKWALLPAPER, 0, (PVOID)img_path.c_str(), SPIF_UPDATEINIFILE);
  if (!status) {
    throw std::runtime_error("Set desktop wallpaper failed: " +
                             std::to_string(GetLastError()));
  }
}

int main(int argc, char* argv[]) {
  std::string lcid = "en-US";
  if (argc > 1) {
    lcid = argv[1];
  }

  asio::io_context ioc{1};
  asio::ssl::context ctx(asio::ssl::context::tlsv12_client);
  ctx.set_verify_mode(asio::ssl::verify_none);

  try {
    auto res = GetScreenResolution();
    auto img_data = DownloadImage(ioc, ctx, res, lcid);
    auto img_path = SaveImage(img_data);
    SetDesktopWallpaper(img_path);
  } catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
    system("pause");
  }
}

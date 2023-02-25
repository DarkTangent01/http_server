## Simple HTTP Server with inotify

This is a simple HTTP server written in C++ that servers static files and uses inotify to detect changes to a specific file and trigger a reload of the page in the browser.

## Dependencies

- `g++` or another C++ compiler
- `make`
- Linux system headers
- `libstdc++` standard library
- `inotify-tools` package for inotify

## Usage

1. Clone the repository
2. `cd` into the project directory
3. Run `make` to build the binary
4. Run the binary with `./http_server`
5. Navigate to `http://localhost:8000` in your browser
6. Edit the `index.html` file and save it.
7. The server should detect the change and reload the page in your browser

## Configuration

By default, the server listens on port `8080` and serves file from the current directory. You can change these settings by modifying the following lines in `http_server.cpp`:

```
#define PORT 8080
...
std::string index_file_path = "index.html";
```

To add support for additional MIME types, modify the `MIME_TYPES` map in the `http_server.cpp`.

```
std::map<std::string, std::string> MIME_TYPES ={
    {"html", "text/html"},
    {"css", "text/css"},
    {"js", "text/javascript"}};
```

## Project status

Under Development

## License

This project is licensed under the MIT License.

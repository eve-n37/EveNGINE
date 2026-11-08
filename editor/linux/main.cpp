// EveNGINE Studio for Linux: hosts the HTML editor in WebKitGTK and answers the same native
// bridge actions as the macOS shell (editor/native/main.m).
#include "export.hpp"
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <csignal>
#include <deque>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
namespace fs = std::filesystem;
namespace {
struct Studio {
  fs::path resources, recovery;
  GtkWidget *window = nullptr;
  WebKitWebView *web = nullptr;
  bool unsaved = false, loaded = false;
  std::string pendingPath;
  std::set<GPid> playtests;
  // Automated tests: file dialogs take answers from this queue instead of opening, and an
  // empty queue cancels.
  bool testing = false;
  std::deque<std::string> testChoices;
};
Studio studio;

std::string quote(const std::string &s) {
  std::string out = "\"";
  for (unsigned char c : s) {
    if (c == '"' || c == '\\')
      out += '\\', out += char(c);
    else if (c == '\n')
      out += "\\n";
    else if (c < 0x20) {
      char buffer[8];
      snprintf(buffer, sizeof buffer, "\\u%04x", c);
      out += buffer;
    } else
      out += char(c);
  }
  return out + "\"";
}
std::string readFile(const fs::path &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in)
    throw std::runtime_error("Cannot open " + path.filename().string());
  std::stringstream data;
  data << in.rdbuf();
  return data.str();
}
void writeFile(const fs::path &path, const std::string &text) {
  auto temp = path;
  temp += ".tmp";
  {
    std::ofstream out(temp, std::ios::binary);
    out << text;
    out.close();
    if (!out)
      throw std::runtime_error("Cannot write " + path.filename().string());
  }
  fs::rename(temp, path);
}
// Checks that text is a JSON object and returns it re-serialized, which is safe to embed in
// a script.
std::string canonicalJson(const std::string &text) {
  JSCContext *context = jsc_context_new();
  JSCValue *value = jsc_value_new_from_json(context, text.c_str());
  bool ok = value && jsc_value_is_object(value) && !jsc_value_is_array(value) &&
            !jsc_context_get_exception(context);
  std::string out;
  if (ok) {
    char *json = jsc_value_to_json(value, 0);
    out = json ? json : "";
    g_free(json);
  }
  if (value)
    g_object_unref(value);
  g_object_unref(context);
  if (out.empty())
    throw std::runtime_error("This file is not an EveNGINE project.");
  return out;
}
std::string property(JSCValue *object, const char *name) {
  JSCValue *v = jsc_value_object_get_property(object, name);
  std::string out;
  if (v && jsc_value_is_string(v)) {
    char *s = jsc_value_to_string(v);
    out = s;
    g_free(s);
  }
  if (v)
    g_object_unref(v);
  return out;
}
// Serializes a property (such as the project) as JSON; empty if missing.
std::string propertyJson(JSCValue *object, const char *name, int indent = 0) {
  JSCValue *v = jsc_value_object_get_property(object, name);
  std::string out;
  if (v && jsc_value_is_object(v)) {
    char *s = jsc_value_to_json(v, guint(indent));
    out = s ? s : "";
    g_free(s);
  }
  if (v)
    g_object_unref(v);
  return out;
}
std::string projectJson(JSCValue *body) {
  JSCValue *project = jsc_value_object_get_property(body, "project");
  bool ok = project && jsc_value_is_object(project) && property(project, "format") == "lota-studio";
  if (project)
    g_object_unref(project);
  if (!ok)
    throw std::runtime_error("Invalid project");
  return propertyJson(body, "project", 2) + "\n";
}

void run(const std::string &script) {
  webkit_web_view_evaluate_javascript(studio.web, script.c_str(), -1, nullptr, nullptr, nullptr,
                                      nullptr, nullptr);
}
void reply(const std::string &request, const std::string &data) {
  run("window.studioReply(" + request + "," + data + ")");
}

struct Filter {
  const char *name;
  std::vector<const char *> patterns;
};
// Shows a file dialog and returns the chosen path, or an empty string when cancelled.
std::string choose(GtkFileChooserAction action, const char *title,
                   const std::vector<Filter> &filters, const std::string &suggested = "",
                   const char *extension = nullptr) {
  if (studio.testing) {
    if (studio.testChoices.empty())
      return "";
    auto path = studio.testChoices.front();
    studio.testChoices.pop_front();
    if (!path.empty() && extension && fs::path(path).extension().empty())
      path += extension;
    return path;
  }
  GtkFileChooserNative *dialog = gtk_file_chooser_native_new(
      title, GTK_WINDOW(studio.window), action,
      action == GTK_FILE_CHOOSER_ACTION_OPEN ? "_Open" : "_Save", "_Cancel");
  auto chooser = GTK_FILE_CHOOSER(dialog);
  for (auto &f : filters) {
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, f.name);
    for (auto pattern : f.patterns)
      gtk_file_filter_add_pattern(filter, pattern);
    gtk_file_chooser_add_filter(chooser, filter);
  }
  if (action == GTK_FILE_CHOOSER_ACTION_SAVE) {
    gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
    gtk_file_chooser_set_current_name(chooser, suggested.c_str());
  }
  std::string path;
  if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    char *name = gtk_file_chooser_get_filename(chooser);
    path = name ? name : "";
    g_free(name);
  }
  g_object_unref(dialog);
  if (!path.empty() && extension && fs::path(path).extension().empty())
    path += extension;
  return path;
}

fs::path temporaryProject(const std::string &json) {
  char *folder = g_dir_make_tmp("evengine-playtest-XXXXXX", nullptr);
  if (!folder)
    throw std::runtime_error("Cannot create a temporary folder");
  fs::path dir = folder;
  g_free(folder);
  writeFile(dir / "project.lota", json);
  fs::copy_file(studio.resources / "font.pam", dir / "font.pam");
  return dir / "project.lota";
}
void validate(const fs::path &project) {
  std::string runtime = (studio.resources / "runtime").string(), path = project.string();
  const char *argv[] = {runtime.c_str(), "--project", path.c_str(), "--validate", nullptr};
  char *out = nullptr, *err = nullptr;
  int status = 0;
  GError *problem = nullptr;
  bool started = g_spawn_sync(nullptr, const_cast<char **>(argv), nullptr, G_SPAWN_DEFAULT, nullptr,
                              nullptr, &out, &err, &status, &problem);
  std::string message = problem ? problem->message : err ? err : "";
  g_free(out);
  g_free(err);
  if (problem)
    g_error_free(problem);
  if (!started || !g_spawn_check_wait_status(status, nullptr))
    throw std::runtime_error(message.empty() ? "Runtime rejected project" : message);
}
void playtestExited(GPid pid, gint, gpointer) {
  studio.playtests.erase(pid);
  g_spawn_close_pid(pid);
}
void playtest(const fs::path &project, const std::string &scene) {
  std::string runtime = (studio.resources / "runtime").string(), path = project.string();
  const char *argv[] = {runtime.c_str(), "--project",   path.c_str(),
                        "--scene",       scene.c_str(), nullptr};
  GPid pid = 0;
  GError *problem = nullptr;
  if (!g_spawn_async(nullptr, const_cast<char **>(argv), nullptr, G_SPAWN_DO_NOT_REAP_CHILD,
                     nullptr, nullptr, &pid, &problem)) {
    std::string message = problem->message;
    g_error_free(problem);
    throw std::runtime_error(message);
  }
  studio.playtests.insert(pid);
  g_child_watch_add(pid, playtestExited, nullptr);
}

void handle(JSCValue *body) {
  JSCValue *idValue = jsc_value_object_get_property(body, "id");
  char *idJson = idValue ? jsc_value_to_json(idValue, 0) : nullptr;
  std::string request = idJson ? idJson : "null";
  g_free(idJson);
  if (idValue)
    g_object_unref(idValue);
  auto action = property(body, "action");
  const std::string cancelled = "{\"cancelled\":true}";
  try {
    if (action == "open") {
      auto path = choose(GTK_FILE_CHOOSER_ACTION_OPEN, "Open project",
                         {{"EveNGINE projects", {"*.eve", "*.lota"}},
                          {"Object templates", {"*.eve-object", "*.lota-object"}}});
      if (path.empty())
        return reply(request, cancelled);
      auto project = canonicalJson(readFile(path));
      studio.unsaved = false;
      return reply(request, "{\"project\":" + project + ",\"path\":" + quote(path) + "}");
    }
    if (action == "save" || action == "autosave") {
      fs::path path;
      if (action == "autosave") {
        path = studio.recovery;
        studio.unsaved = true;
      } else if (!property(body, "path").empty())
        path = property(body, "path");
      else {
        path = choose(GTK_FILE_CHOOSER_ACTION_SAVE, "Save project",
                      {{"EveNGINE projects", {"*.eve", "*.lota"}}}, "My Adventure.eve", ".eve");
        if (path.empty())
          return reply(request, cancelled);
      }
      writeFile(path, projectJson(body));
      if (action == "save")
        studio.unsaved = false;
      return reply(request, "{\"path\":" + quote(path.string()) + "}");
    }
    if (action == "recover") {
      auto project = canonicalJson(readFile(studio.recovery));
      studio.unsaved = true;
      return reply(request, "{\"project\":" + project + "}");
    }
    if (action == "template") {
      auto path =
          choose(GTK_FILE_CHOOSER_ACTION_SAVE, "Save object template",
                 {{"Object templates", {"*.eve-object"}}}, property(body, "name"), ".eve-object");
      if (path.empty())
        return reply(request, cancelled);
      writeFile(path, propertyJson(body, "template", 2) + "\n");
      return reply(request, "{\"message\":\"Object template saved.\"}");
    }
    if (action == "png") {
      auto path = choose(GTK_FILE_CHOOSER_ACTION_SAVE, "Export PNG", {{"PNG images", {"*.png"}}},
                         property(body, "name"), ".png");
      if (path.empty())
        return reply(request, cancelled);
      gsize length = 0;
      guchar *bytes = g_base64_decode(property(body, "data").c_str(), &length);
      std::string data(reinterpret_cast<char *>(bytes), length);
      g_free(bytes);
      if (data.empty())
        throw std::runtime_error("Invalid PNG data");
      writeFile(path, data);
      return reply(request, "{\"message\":\"PNG saved.\"}");
    }
    if (action == "play" || action == "export") {
      auto json = projectJson(body);
      auto project = temporaryProject(json);
      validate(project);
      if (action == "play") {
        auto scene = property(body, "scene");
        if (scene.empty()) {
          JSCValue *p = jsc_value_object_get_property(body, "project");
          scene = property(p, "start");
          g_object_unref(p);
        }
        playtest(project, scene);
        return reply(request, "{\"message\":\"Native playtest launched. F3 shows collision; "
                              "Esc opens Pause.\"}");
      }
      auto destination = choose(GTK_FILE_CHOOSER_ACTION_SAVE, "Export Linux game",
                                {{"Game folders", {"*"}}}, "My Adventure");
      if (destination.empty())
        return reply(request, cancelled);
      std::string error;
      eve::LinuxExportSources sources{(studio.resources / "runtime").string(),
                                      (studio.resources / "font.pam").string(),
                                      (studio.resources / "lib" / "libSDL3.so.0").string(),
                                      (studio.resources / "studio.png").string()};
      if (!eve::exportLinuxGame(json, sources, destination, error))
        throw std::runtime_error(error);
      return reply(request, "{\"message\":" +
                                quote("Exported " + fs::path(destination).filename().string() +
                                      ". Run it from that folder.") +
                                "}");
    }
    throw std::runtime_error("Unknown editor action");
  } catch (const std::exception &e) {
    reply(request, "{\"error\":" + quote(e.what()) + "}");
  }
}
void received(WebKitUserContentManager *, WebKitJavascriptResult *result, gpointer) {
  JSCValue *body = webkit_javascript_result_get_js_value(result);
  if (jsc_value_is_object(body))
    handle(body);
}

void loadPending() {
  if (studio.pendingPath.empty() || !studio.loaded)
    return;
  try {
    auto project = canonicalJson(readFile(studio.pendingPath));
    run("window.studioLoad(" + project + "," + quote(studio.pendingPath) + ")");
  } catch (const std::exception &e) {
    run("notice(" + quote(e.what()) + ")");
  }
  studio.pendingPath.clear();
}
void loadChanged(WebKitWebView *, WebKitLoadEvent event, gpointer) {
  if (event != WEBKIT_LOAD_FINISHED)
    return;
  studio.loaded = true;
  loadPending();
  if (auto script = std::getenv("EVE_STUDIO_TEST_SCRIPT")) {
    try {
      run(readFile(script));
    } catch (const std::exception &e) {
      std::cerr << e.what() << '\n';
      gtk_main_quit();
    }
  }
}
gboolean decide(WebKitWebView *, WebKitPolicyDecision *decision, WebKitPolicyDecisionType type,
                gpointer) {
  if (type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION)
    return FALSE;
  auto navigation = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
  auto request = webkit_navigation_action_get_request(
      webkit_navigation_policy_decision_get_navigation_action(navigation));
  std::string uri = webkit_uri_request_get_uri(request);
  char *ui = g_filename_to_uri((studio.resources / "ui").c_str(), nullptr, nullptr);
  bool local = ui && uri.rfind(std::string(ui) + "/", 0) == 0;
  g_free(ui);
  if (local)
    webkit_policy_decision_use(decision);
  else
    webkit_policy_decision_ignore(decision);
  return TRUE;
}
gboolean closing(GtkWidget *, GdkEvent *, gpointer) {
  if (!studio.unsaved || std::getenv("EVE_STUDIO_TEST_SCRIPT"))
    return FALSE;
  GtkWidget *dialog =
      gtk_message_dialog_new(GTK_WINDOW(studio.window), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
                             GTK_BUTTONS_NONE, "Close EveNGINE Studio?");
  gtk_message_dialog_format_secondary_text(
      GTK_MESSAGE_DIALOG(dialog),
      "Use Save to keep a portable project. Your latest edits are also kept in local recovery.");
  gtk_dialog_add_buttons(GTK_DIALOG(dialog), "_Keep editing", GTK_RESPONSE_CANCEL, "_Close",
                         GTK_RESPONSE_ACCEPT, nullptr);
  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
  bool close = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT;
  gtk_widget_destroy(dialog);
  return !close;
}
// Test hook: window.studioTestDone(text) prints the text and quits.
void testDone(WebKitUserContentManager *, WebKitJavascriptResult *result, gpointer) {
  char *text = jsc_value_to_string(webkit_javascript_result_get_js_value(result));
  std::cout << text << std::endl;
  g_free(text);
  gtk_main_quit();
}

fs::path executableFolder() {
  std::error_code error;
  auto self = fs::read_symlink("/proc/self/exe", error);
  return error ? fs::current_path() : self.parent_path();
}
// The AppImage starts Studio from inside its own folder, so paths typed on the command line are
// resolved against the folder the user started it from (see tools/package_appimage.py).
fs::path userPath(const std::string &path) {
  fs::path p(path);
  if (auto from = std::getenv("EVE_STUDIO_WORKDIR"); from && p.is_relative())
    return fs::path(from) / p;
  return fs::absolute(p);
}
int usage() {
  std::cerr << "Usage: evengine-studio [project.eve]\n"
               "       evengine-studio --export <project.eve> <game folder>\n"
               "Options: --resources <folder>   use another resources folder\n";
  return 2;
}
} // namespace

int main(int argc, char **argv) {
  studio.resources = executableFolder() / "resources";
  if (auto env = std::getenv("EVE_STUDIO_RESOURCES"))
    studio.resources = env;
  std::vector<std::string> args;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "--resources" && i + 1 < argc)
      studio.resources = argv[++i];
    else if (a == "--version") {
      std::cout << "EveNGINE Studio 1.1.0\n";
      return 0;
    } else if (a == "--help" || a == "-h")
      return usage(), 0;
    else
      args.push_back(a);
  }
  studio.resources = fs::absolute(studio.resources);
  if (!args.empty() && args[0] == "--export") {
    if (args.size() != 3)
      return usage();
    args[1] = userPath(args[1]).string();
    args[2] = userPath(args[2]).string();
    try {
      auto json = canonicalJson(readFile(args[1]));
      auto project = temporaryProject(json);
      validate(project);
      fs::remove_all(project.parent_path());
      std::string error;
      eve::LinuxExportSources sources{(studio.resources / "runtime").string(),
                                      (studio.resources / "font.pam").string(),
                                      (studio.resources / "lib" / "libSDL3.so.0").string(),
                                      (studio.resources / "studio.png").string()};
      if (!eve::exportLinuxGame(readFile(args[1]), sources, args[2], error))
        throw std::runtime_error(error);
      std::cout << "Exported " << args[2] << '\n';
      return 0;
    } catch (const std::exception &e) {
      std::cerr << e.what() << '\n';
      return 1;
    }
  }
  if (args.size() > 1 || (!args.empty() && args[0].rfind("--", 0) == 0))
    return usage();
  if (!args.empty())
    studio.pendingPath = userPath(args[0]).string();
  if (!fs::is_regular_file(studio.resources / "ui" / "index.html")) {
    std::cerr << "EveNGINE Studio resources are missing: " << studio.resources << '\n';
    return 1;
  }
  if (auto choices = std::getenv("EVE_STUDIO_TEST_CHOOSE")) {
    studio.testing = true;
    std::stringstream list(choices);
    for (std::string path; std::getline(list, path, ':');)
      studio.testChoices.push_back(userPath(path).string());
  }
  gtk_init(&argc, &argv);
  g_set_application_name("EveNGINE Studio");
  auto data = fs::path(g_get_user_data_dir()) / "EveNGINE Studio";
  fs::create_directories(data);
  studio.recovery = data / "recovery.lota";

  WebKitUserContentManager *content = webkit_user_content_manager_new();
  g_signal_connect(content, "script-message-received::studio", G_CALLBACK(received), nullptr);
  webkit_user_content_manager_register_script_message_handler(content, "studio");
  if (std::getenv("EVE_STUDIO_TEST_SCRIPT")) {
    g_signal_connect(content, "script-message-received::studioTestDone", G_CALLBACK(testDone),
                     nullptr);
    webkit_user_content_manager_register_script_message_handler(content, "studioTestDone");
  }
  studio.web = WEBKIT_WEB_VIEW(webkit_web_view_new_with_user_content_manager(content));
  WebKitSettings *settings = webkit_web_view_get_settings(studio.web);
  webkit_settings_set_allow_file_access_from_file_urls(settings, TRUE);
  webkit_settings_set_enable_developer_extras(settings,
                                              std::getenv("EVE_STUDIO_DEVTOOLS") != nullptr);
  g_signal_connect(studio.web, "load-changed", G_CALLBACK(loadChanged), nullptr);
  g_signal_connect(studio.web, "decide-policy", G_CALLBACK(decide), nullptr);

  studio.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(studio.window), "EveNGINE Studio");
  gtk_window_set_default_size(GTK_WINDOW(studio.window), 1360, 900);
  gtk_widget_set_size_request(studio.window, 1080, 700);
  gtk_window_set_icon_from_file(GTK_WINDOW(studio.window),
                                (studio.resources / "studio.png").c_str(), nullptr);
  gtk_container_add(GTK_CONTAINER(studio.window), GTK_WIDGET(studio.web));
  g_signal_connect(studio.window, "delete-event", G_CALLBACK(closing), nullptr);
  g_signal_connect(studio.window, "destroy", G_CALLBACK(gtk_main_quit), nullptr);
  gtk_widget_show_all(studio.window);
  char *uri = g_filename_to_uri((studio.resources / "ui" / "index.html").c_str(), nullptr, nullptr);
  webkit_web_view_load_uri(studio.web, uri);
  g_free(uri);
  gtk_main();
  // Closing Studio also closes its playtests, as on macOS.
  for (auto pid : studio.playtests)
    kill(pid, SIGTERM);
  return 0;
}

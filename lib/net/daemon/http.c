/*    /daemon/http.c
 *    from Nightmare IV
 *    an http daemon that can talk to Mosaic and other WWW clients
 *    created by Descartes of Borg 940521
 *    callback fix by Robocoder 950117
 *    modified for discworld Pinkfish 10th of March 1995
 */

#include <socket.h>
#include "http.h"

#define PORT_HTTP 5678
#define SAVE_FILE "/net/save/http"

#undef DEBUG
#ifdef DEBUG
#define TP(STR) if (find_player("pinkfish")) tell_object(find_player("pinkfish"), STR)
#else
#define TP(STR)
#endif

#define RES_DIR    0
#define RES_FILE   1
#define RES_NORMAL 2

//inherit DAEMON;

static private int __SocketHTTP, request_no, current_no_reads;
static private mapping __Sockets, __Activity, Pos;
static string current_file;
mapping http_stats;
void send_file(int fd, string str);
void send_string(int fd, string str, int last_mod);

/* States... */
#define HTTP_WAITING 0
#define HTTP_IN_HEADER 1
#define HTTP_SEND_BODY 2
#define HTTP_SEND_HEADER 3

/* Bits in the fd array */
#define FNAME 0
#define STATE 1
#define HEADER 2
#define START_CMD 3
#define STRING 4
#define SIZE 5
#define CUR_POS 6

/* Command types */
#define HTTP_GET_COMMAND 0

void create() {
//    daemon::create();
//    set_no_clean(1);
  __Sockets = ([]);
  __Activity = ([]);
  http_stats = ([ ]);
  Pos = ([ ]);
  call_out("setup", 2);
  call_out("clean_sockets", 180);
  seteuid(getuid());
  restore_object(SAVE_FILE);
} /* create() */

static void setup() {
  if((__SocketHTTP=socket_create(STREAM,"read_callback","close_callback"))<0){
    log_file("httpd", "Failed to create socket.\n");
    return;
  }
  if(socket_bind(__SocketHTTP, PORT_HTTP) < 0) {
    socket_close(__SocketHTTP);
    log_file("httpd", "Failed to listen to socket.\n");
    return;
  }
  if(socket_listen(__SocketHTTP, "listen_callback") < 0) {
    socket_close(__SocketHTTP);
    log_file("httpd", "Failed to listen to socket.\n");
  }
} /* setup() */

int hex_to_decimal(string str) {
  int ret, pos, len;

  ret = 0;
  pos = 0;
  len = strlen(str);
  while (pos < len) {
    ret = ret*16;
    switch (str[pos]) {
      case '1' :
      case '2' :
      case '3' :
      case '4' :
      case '5' :
      case '6' :
      case '7' :
      case '8' :
      case '9' :
        ret += str[pos]-'0';
        break;
      case 'a' :
      case 'b' :
      case 'c' :
      case 'd' :
      case 'e' :
      case 'f' :
        ret += str[pos]-'a'+10;
        break;
      case 'A' :
      case 'B' :
      case 'C' :
      case 'D' :
      case 'E' :
      case 'F' :
        ret += str[pos]-'A'+10;
        break;
    }
    pos++;
  }
  return ret;
} /* hex_to_decimal() */

/*
 * Replace all the mime codes in the address
 */
string unmime(string str) {
  string *bits, start;
  int i;

  bits = explode("#"+str, "%");
  bits[0] = bits[0][1..10000];
  for (i=1;i<sizeof(bits);i++) {
    start = bits[i][0..1];
    bits[i] = sprintf("%c%s", hex_to_decimal(start),
                              bits[i][2..10000]);
  }
  return implode(bits, "");
} /* unmime() */

void listen_callback(int fd) {
  int neu;

  if((neu = socket_accept(fd, "read_callback", "write_callback")) < 0)
    return;
  map_delete(Pos, neu);
  Pos[neu] = ([ ]);
  Pos[neu][HEADER] = "\n";
  Pos[neu][FNAME] = "";
  write_callback(neu);
} /* listen_callback() */

void write_callback(int fd) {
  string tmp;
  int x;

  if(!__Sockets[fd]) {
    sscanf(socket_address(fd), "%s %*s", tmp);
    x = resolve(tmp, "resolve_incoming");
    __Sockets[fd] = ([ "address": tmp, "key": x, "time": time() ]);
    add_activity(fd, sprintf("CONNECTED (%s)\n", ctime(time())));
  }

  if (Pos[fd][STATE] == HTTP_SEND_HEADER ||
      Pos[fd][STATE] == HTTP_SEND_BODY)
    retry(fd);
} /* write_callback() */

void read_callback(int fd, string str) {
  string cmd, args, *bits;
  int got_response, i;
  string fname, value;

  TP("Received: "+str);
/* There is something being done on this connection already */
  switch (Pos[fd][STATE]) {
    case HTTP_WAITING :
      Pos[fd][FNAME] += str;
      if (sscanf(Pos[fd][FNAME], "%*s\n%*s")) {
        str = Pos[fd][FNAME];
        if(!str || str == "") {
          http_error(fd, BAD_CMD);
          got_response = 1;
        } else {
          bits = explode(replace_string(str, "\r", ""), "\n");
          if (bits)
            str = bits[0];
          else
            str = "";
          sscanf(str, "%s %s", cmd, args);
          switch(lower_case(cmd)) {
            case "get":
              Pos[fd][STATE] = HTTP_IN_HEADER;
              Pos[fd][FNAME] = explode(args, " ")[0];
              Pos[fd][START_CMD] = HTTP_GET_COMMAND;
              TP(lower_case(explode(args+" bing", " ")[1]));
              if (lower_case(explode(args+" bing", " ")[1]) != "http/1.0") {
                read_callback(fd, "\n");
              } else if (sizeof(bits) > 1) {
                read_callback(fd, "Rubbish: bing\n"+implode(bits[1..10000], "\n"));
              }
              break;
            default:
              if (Pos[fd][STATE] == HTTP_WAITING) {
                http_error(fd, BAD_CMD);
              }
              break;
          }
        }
      }
      break;
    case HTTP_IN_HEADER :
      Pos[fd][HEADER] += replace_string(str, "\r\n", "\n");
      if (member_array("", explode("#"+Pos[fd][HEADER]+"#", "\n"))) {
/* Found the end */
#ifdef PROCESS_HEADER
/* Currently we do nothing with the values in the header */
        Pos[fd][HEADER] = unmime(Pos[fd][HEADER]);
        bits = explode(Pos[fd][HEADER], "\n");
        if (sizeof(bits) > 1) {
          for (i=0;i<sizeof(bits);i++) {
            if (bits[i][0] == '&') {
/* field value */
              sscanf(bits[i], "&%s: %s", fname, value);
              Pos[fd][FIELDS][fname] = value;
            } else {
              //sscanf(bits[i], "%s: %s", cmd, args);
              //switch (lower_case(cmd)) {
              //}
            }
          }
        }
#endif
        switch (Pos[fd][START_CMD]) {
          case HTTP_GET_COMMAND :
            get_file(fd, Pos[fd][FNAME]);
            break;
        }
      }
      break;
  }
  if (got_response)
    retry(fd);
} /* read_callback() */

void close_callback(int fd) {
  log_file("httpd", "Socket closed.\n");
  socket_close(fd);
  if(fd == __SocketHTTP)
    this_object()->remove();
} /* close_callback() */

void resolve_incoming(string nom, string addr, int cle) {
  int *fds;
  int i;

  i = sizeof(fds = keys(__Sockets));
  while(i--)
    if(__Sockets[fds[i]]["key"]==cle && __Sockets[fds[i]]["address"]==addr) {
        __Sockets[fds[i]]["name"] = (nom ? nom : addr);
        return;
    }
  log_file("httpd",sprintf("Ip %s resloved to %s after connection closed.\n",
           addr, (nom ? nom : "NOT RESOLVED")));
} /* resolve_incoming( */

static private void http_error(int fd, mapping err) {
  string str;

  TP("http_error"+err["error"]+", "+err["file"]+"\n");
  add_activity(fd, sprintf("ERROR: %s (%s)\n", err["error"], ctime(time())));
  send_string(fd,
        (str = read_file(err["file"])) ? str : "<html><head><title>Error"
        "</title></head><body><h2>Error!</h2>This is the end of the line.  The "
        "tomatoes have finaly expired.</body></html>\n",
        stat(err["file"])[1]);
} /* http_error() */

static private void add_activity(int fd, string act) {
  if(!__Activity[fd]) __Activity[fd] = ({ act });
  else __Activity[fd] += ({ act });
} /* add_activity() */

void close_connection(int fd) {
  int i, maxi;

  maxi = sizeof(__Activity[fd]);
  if(!__Sockets[fd]["name"]) __Sockets[fd]["name"]=__Sockets[fd]["address"];
  for(i =0; i<maxi; i++)
    log_file("httpd", sprintf("%s: %s",
      __Sockets[fd]["name"], __Activity[fd][i]));
  map_delete(__Sockets, fd);
  map_delete(__Activity, fd);
  map_delete(Pos, fd);
  socket_close(fd);
} /* close_connection() */

static void clean_sockets() {
  int *cles;
  int i;

  i = sizeof(cles = keys(__Sockets));
  while(i--)
    if(time() - __Sockets[cles[i]]["time"] > 180) close_connection(cles[i]);
  call_out("clean_sockets", 180);
} /* clean_sockets() */

string *www_resolve(string file) {
  string *parts, dir, normal, tmp;

  parts = explode(file, "/") - ({ ".", "..", "" });
  if(!sizeof(parts)) {
    file = sprintf("%s/index.html", DIR_WWW);
    dir = DIR_WWW;
    normal = "/index.html";
  } else if(parts[0][0] == '~' && strlen(parts[0]) > 2) {
    if (sizeof(parts) == 1) {
      parts += ({ "index.html" });
    }
    dir = parts[0];
    if (dir[1] >= 'a' && dir[1] < 'z') {
      parts[0] = sprintf("/w/%s/public_html",
        dir[1..1000]);
    } else {
      parts[0] = sprintf("/d/%s/public_html",
        lower_case(dir)[1..1000]);
    }
    normal = dir+"/"+implode(parts[1..1000], "/");
    file = implode(parts, "/");
    dir = implode(parts[0..sizeof(parts)-2], "/");
  } else if (parts[0] == "w") {
    file = sprintf("%s/index.html", DIR_WWW);
    dir = DIR_WWW+"/";
    normal = "/index.html";
  } else if (parts[0] == "www") {
    file = DIR_WWW+"/"+implode(parts[1..1000], "/");
    dir = DIR_WWW+"/"+implode(parts[1..sizeof(parts)-2], "/");
    normal = "/"+implode(parts[1..1000], "/");
  } else {
    file = DIR_WWW+"/"+implode(parts, "/");
    dir = DIR_WWW+"/"+implode(parts[0..sizeof(parts)-2], "/");
    normal = "/"+implode(parts, "/");
  }
  if(file_size(file) == -2) {
    dir = file;
    file += "/index.html";
    normal += "/index.html";
  }
  return ({ dir, file, normal });
} /* www_resolve() */

static private void get_file(int fd, string file) {
  string *parts, *tmp, *bits, dir;
  mixed str;
  string id, args;
  int x, i;
  mapping fields;

  TP("Started get.\n");
  fields = ([ ]);
  if (sscanf(file, "%s?%s", file, args)) {
    bits = explode(args, "&");
    for (i=0;i<sizeof(bits);i++) {
      sscanf(bits[i], "%s=%s", id, str);
      id = unmime(id);
      fields[id] = unmime(replace(str, "+", " "));
    }
    args = 0;
  }
  add_activity(fd, sprintf("GET %s (%s)\n", file, ctime(time())));
  file = unmime(file);
  sscanf(file, "%s___%s", file, args);
/*
  if (!args) {
    file = (tmp = explode(file, " "))[0];
  } else
    args = (tmp = explode(args, " "))[0];
 */

  if (!args)
    args = "";
  TP("File = "+file+" Args = "+args+"\n");
  if (file[0] != '/')
    file = sprintf("/%s", file);

  parts = www_resolve(file);
  file = parts[RES_FILE];
  dir = parts[RES_DIR];

  if (file_size(file) <= 0) {
    TP("File not found"+file+"\n");
    http_error(fd, NOT_FOUND);
    return;
  } else {
    current_no_reads = ++http_stats[parts[RES_NORMAL]];
    current_file = file;
    if (request_no++ == 10) {
      save_object(SAVE_FILE);
      request_no = 0;
    }
    if (dir[strlen(dir)-1] != '/')
      dir += "/";

TP("Found file "+file+"\n");
    parts = explode(file, ".");
    if (parts[sizeof(parts)-1] == "c") {
      int mod_date;

TP("Attempting a call.");
      if (catch(str = (string)call_other(file, "www_request", args,
                fields))) {
        http_error(fd, BAD_GATEWAY);
        return ;
      }
      mod_date = stat(file)[1];
      if (pointerp(str) && intp(str[0])) {
        if (sizeof(str) < 2) {
          http_error(fd, BAD_GATEWAY);
          return ;
        }
        mod_date = str[0];
        str = str[1];
      }
      if (str && (stringp(str) || bufferp(str)))
        send_string(fd, str, mod_date);
      else {
        http_error(fd, BAD_GATEWAY);
        return ;
      }
    } else {
      string fname;

      TP("Reading buffer.\n");
      str = read_file(file, 0, 1);
      fname = file;
      TP("Start = "+lower_case(str[0..5])+"\n");
      if (lower_case(str[0..5]+"") == "<html>") {
        TP("Found html file.\n");
        str = read_file(file);
        bits = explode(str, "@@");
        str = "";
        for (i=0;i<sizeof(bits);i+=2) {
          str += bits[i];
          if (i+1 < sizeof(bits)) {
            sscanf(bits[i+1], "%s:%s", file, args);
            TP("Calling "+file+"->www_function( \""+args+"\" )\n");
            if (file[0] != '/' && file[0] != '~')
              file = dir+file;
            parts = www_resolve(file);
            bits[i+1] = "Oook!  Error!";
            if (catch(bits[i+1] = ((parts[RES_FILE])->www_function(args,
                                  fields)))) {
              str += "Error!";
            } else {
              str += bits[i+1];
            }
          }
        }
        send_string(fd, str, stat(fname)[1]);
      } else {
        send_file(fd, file);
        str = "kjsdfhkjsh";
      }
    }
  }
  if (!str || (!stringp(str) && !bufferp(str)))
    http_error(fd, NOT_FOUND);
} /* get_file() */

void retry(int fd) {
  int res, size, i;
  string *bits, str;

  do {
    if (Pos[fd][STATE] == HTTP_SEND_HEADER) {
      str = Pos[fd][HEADER];
    } else if (Pos[fd][FNAME] != "STRING FILE") {
      if (Pos[fd][SIZE] < Pos[fd][CUR_POS]+1024)
        str = read_buffer(Pos[fd][FNAME], Pos[fd][CUR_POS],
                          Pos[fd][SIZE]-Pos[fd][CUR_POS]);
      else
        str = read_buffer(Pos[fd][FNAME], Pos[fd][CUR_POS], 1024);
      Pos[fd][CUR_POS] += 1024;
      TP("Getting file "+Pos[fd][FNAME]+" from "+Pos[fd][CUR_POS]+"\n");
    } else {
      str = Pos[fd][STRING][Pos[fd][CUR_POS]..Pos[fd][CUR_POS]+1024];
      Pos[fd][CUR_POS] += 1025;
    }
  
    if((res = socket_write(fd, str)) != EECALLBACK) {
      if(res == EEWOULDBLOCK) {
        call_out("retry", 1, fd);
        if (Pos[fd])
            Pos[fd][CUR_POS] -= 1024;
        } else if (res == EESUCCESS && Pos[fd][CUR_POS] > Pos[fd][SIZE]) {
          close_connection(fd);
        } else if (res == EESUCCESS) {
          if (Pos[fd][STATE] == HTTP_SEND_HEADER)
            Pos[fd][STATE] = HTTP_SEND_BODY;
        }
    }
  } while (res == EESUCCESS && Pos[fd]);
} /* retry() */
    

int dest_me() {
  socket_close(__SocketHTTP);
  save_object(SAVE_FILE);
  destruct(this_object());
} /* dest_me() */

string format_date(int x) {
  string *parts;
  string str;
  string mon;

  if (x<0 || !intp(x))
    return "Bad time";
  parts = explode(replace_string(ctime(x), "  ", " "), " ");
  switch(parts[0]) {
      case "Mon": str = "Monday, "; break;
      case "Tue": str = "Tuesday, "; break;
      case "Wed": str = "Wednesday, "; break;
      case "Thu": str = "Thursday, "; break;
      case "Fri": str = "Friday, "; break;
      case "Sat": str = "Saturday, "; break;
      case "Sun": str = "Sunday, "; break;
  }
  switch (parts[1]) {
    case "Jun" : 
      mon = "June";
      break;
    case "Jul" :
      mon = "July";
      break;
    case "Oct" :
      mon = "October";
      break;
    case "Nov" :
      mon = "November";
      break;
    case "Dec" :
      mon = "December";
      break;
    case "Sep" :
      mon = "September";
      break;
    case "Aug" :
      mon = "August";
      break;
    case "Jan" :
      mon = "Janurary";
      break;
    case "Feb" :
      mon = "Feburary";
      break;
    case "Mar" :
      mon = "March";
      break;
    case "Apr" :
      mon = "April";
      break;
    case "May" :
      mon = "May";
      break;
    default :
      mon = parts[1];
  }
  str = sprintf("%s%s %s %s %s GMT", str, parts[2], mon,
    parts[4][2..3], parts[3]);
  return str;
} /* format_date() */

void send_string(int fd, string str, int mod_date) {
  Pos[fd][FNAME] = "STRING FILE";
  Pos[fd][STRING] = str;
  if (Pos[fd][HEADER] == "\n\n" ||
      Pos[fd][HEADER] == "\n") {
/* Stupid mode, no header */
    Pos[fd][STATE] = HTTP_SEND_BODY;
  } else {
    Pos[fd][STATE] = HTTP_SEND_HEADER;
    Pos[fd][HEADER] =
    "HTTP/1.0 200 Request serviced ok.\r\n"
    "Content-Type: text/html\r\n"
    "Last-Modified: "+ctime(mod_date)+"\r\n"+
    "Content-Length: "+strlen(str)+"\r\n\r\n";
  }
  Pos[fd][SIZE] = strlen(str);
  retry(fd);
} /* send_string() */

void send_file(int fd, string str) {
  string *bits;

  Pos[fd][FNAME] = str;
  Pos[fd][SIZE] = file_size(str);
  if (Pos[fd][HEADER] == "\n\n" ||
      Pos[fd][HEADER] == "\n") {
/* Stupid mode, no header */
    Pos[fd][STATE] = HTTP_SEND_BODY;
  } else {
    Pos[fd][STATE] = HTTP_SEND_HEADER;
    bits = explode(Pos[fd][FNAME], ".");
    Pos[fd][HEADER] = "HTTP/1.0 200 Request serviced ok.\r\n";
    switch (lower_case(bits[sizeof(bits)-1])) {
      case "gif" :
        Pos[fd][HEADER] += "Content-Type: image/gif\r\n";
        break;
      default :
        Pos[fd][HEADER] += "Content-Type: text/html\r\n";
        break;
    }
    Pos[fd][HEADER] += "Last-Modified: "+ctime(stat(str)[1])+"\r\n";
    Pos[fd][HEADER] += "Content-Length: "+file_size(str)+"\r\n\r\n";
  }
  retry(fd);
} /* send_file() */

string query_current_file() {
  return current_file;
} /* query_current_file() */

/* Number of times this file has been accessed */
int query_current_no_reads() {
  return current_no_reads;
} /* query_current_no_reads() */

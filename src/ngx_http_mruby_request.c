/*
// ngx_http_mruby_request.c - ngx_mruby mruby module
//
// See Copyright Notice in ngx_http_mruby_module.c
*/

#include "ngx_http_mruby_request.h"

#include <mruby.h>
#include <mruby/proc.h>
#include <mruby/data.h>
#include <mruby/compile.h>
#include <mruby/string.h>
#include <mruby/class.h>

#define NGX_MRUBY_DEFINE_METHOD_NGX_GET_REQUEST_MEMBER_STR(method_suffix, \
    member) \
static mrb_value ngx_mrb_get_##method_suffix(mrb_state *mrb, mrb_value self);   \
static mrb_value ngx_mrb_get_##method_suffix(mrb_state *mrb, mrb_value self)    \
{ \
  ngx_http_request_t *r = ngx_mrb_get_request(); \
  return mrb_str_new(mrb, (const char *)member.data, member.len); \
}

#define NGX_MRUBY_DEFINE_METHOD_NGX_SET_REQUEST_MEMBER_STR(method_suffix, \
    member) \
static mrb_value ngx_mrb_set_##method_suffix(mrb_state *mrb, mrb_value self); \
static mrb_value ngx_mrb_set_##method_suffix(mrb_state *mrb, mrb_value self)    \
{ \
  mrb_value arg; \
  u_char *str; \
  size_t len; \
  ngx_http_request_t *r; \
  mrb_get_args(mrb, "o", &arg); \
  if (mrb_nil_p(arg)) { \
    return self; \
  } \
  str = (u_char *)mrb_str_to_cstr(mrb, arg); \
  len = RSTRING_LEN(arg); \
  r = ngx_mrb_get_request(); \
  member.len = len; \
  member.data = (u_char *)str; \
  return self; \
}

#define NGX_MRUBY_DEFINE_METHOD_NGX_GET_REQUEST_HEADERS_HASH(direction) \
static mrb_value ngx_mrb_get_request_headers_##direction##_hash( \
    mrb_state *mrb, mrb_value self);  \
static mrb_value ngx_mrb_get_request_headers_##direction##_hash( \
    mrb_state *mrb, mrb_value self) \
{ \
  ngx_list_part_t *part; \
  ngx_table_elt_t *header; \
  ngx_http_request_t *r; \
  ngx_uint_t i; \
  mrb_value hash; \
  mrb_value key; \
  mrb_value value; \
  r = ngx_mrb_get_request(); \
  hash = mrb_hash_new(mrb); \
  part = &(r->headers_##direction.headers.part); \
  header = part->elts; \
  for (i = 0; /* void */; i++) { \
    if (i >= part->nelts) { \
      if (part->next == NULL) { \
        break; \
      } \
      part = part->next; \
      header = part->elts; \
      i = 0; \
    } \
    key = mrb_str_new(mrb, (const char *)header[i].key.data, \
        header[i].key.len); \
    value = mrb_str_new(mrb, (const char *)header[i].value.data, \
        header[i].value.len); \
    mrb_hash_set(mrb, hash, key, value); \
  } \
  return hash; \
}

ngx_http_request_t *ngx_mruby_request = NULL;

static mrb_value ngx_mrb_get_request_header(mrb_state *mrb,
    ngx_list_t *headers);
static mrb_value ngx_mrb_get_request_headers_in(mrb_state *mrb, mrb_value self);
static mrb_value ngx_mrb_get_request_headers_out(mrb_state *mrb,
    mrb_value self);
static ngx_int_t ngx_mrb_set_request_header(mrb_state *mrb, ngx_list_t *headers,
    ngx_uint_t assign);
static mrb_value ngx_mrb_set_request_headers_in(mrb_state *mrb, mrb_value self);
static mrb_value ngx_mrb_set_request_headers_out(mrb_state *mrb,
    mrb_value self);

ngx_int_t ngx_mrb_push_request(ngx_http_request_t *r)
{
  ngx_mruby_request = r;
  return NGX_OK;
}

ngx_http_request_t *ngx_mrb_get_request(void)
{
  return ngx_mruby_request;
}

// request member getter
NGX_MRUBY_DEFINE_METHOD_NGX_GET_REQUEST_MEMBER_STR(request_request_line,
    r->request_line);
NGX_MRUBY_DEFINE_METHOD_NGX_GET_REQUEST_MEMBER_STR(request_uri, r->uri);
NGX_MRUBY_DEFINE_METHOD_NGX_GET_REQUEST_MEMBER_STR(request_unparsed_uri,
    r->unparsed_uri);
NGX_MRUBY_DEFINE_METHOD_NGX_GET_REQUEST_MEMBER_STR(request_method,
    r->method_name);
NGX_MRUBY_DEFINE_METHOD_NGX_GET_REQUEST_MEMBER_STR(request_protocol,
    r->http_protocol);
NGX_MRUBY_DEFINE_METHOD_NGX_GET_REQUEST_MEMBER_STR(request_args, r->args);

// request member setter
NGX_MRUBY_DEFINE_METHOD_NGX_SET_REQUEST_MEMBER_STR(request_request_line,
    r->request_line);
NGX_MRUBY_DEFINE_METHOD_NGX_SET_REQUEST_MEMBER_STR(request_uri, r->uri);
NGX_MRUBY_DEFINE_METHOD_NGX_SET_REQUEST_MEMBER_STR(request_unparsed_uri,
    r->unparsed_uri);
NGX_MRUBY_DEFINE_METHOD_NGX_SET_REQUEST_MEMBER_STR(request_method,
    r->method_name);
NGX_MRUBY_DEFINE_METHOD_NGX_SET_REQUEST_MEMBER_STR(request_protocol,
    r->http_protocol);
NGX_MRUBY_DEFINE_METHOD_NGX_SET_REQUEST_MEMBER_STR(request_args, r->args);

NGX_MRUBY_DEFINE_METHOD_NGX_GET_REQUEST_HEADERS_HASH(in);
NGX_MRUBY_DEFINE_METHOD_NGX_GET_REQUEST_HEADERS_HASH(out);

// TODO:this declation should be moved to headers_(in|out)
NGX_MRUBY_DEFINE_METHOD_NGX_GET_REQUEST_MEMBER_STR(content_type,
    r->headers_out.content_type);
NGX_MRUBY_DEFINE_METHOD_NGX_SET_REQUEST_MEMBER_STR(content_type,
    r->headers_out.content_type);


static mrb_value ngx_mrb_get_request_body(mrb_state *mrb, mrb_value self)
{
  ngx_chain_t *cl;
  size_t len;
  u_char *p;
  u_char *buf;
  ngx_http_request_t *r = ngx_mrb_get_request();

  if (r->request_body == NULL || r->request_body->temp_file
      || r->request_body->bufs == NULL) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "This pahse don't have request_body");
  }

  cl = r->request_body->bufs;

  if (cl->next == NULL) {
    len = cl->buf->last - cl->buf->pos;
    if (len == 0) {
      return mrb_nil_value();
    }

    return mrb_str_new(mrb, (const char *)cl->buf->pos, len);
  }

  len = 0;

  for (; cl; cl = cl->next) {
    len += cl->buf->last - cl->buf->pos;
  }

  if (len == 0) {
    return mrb_nil_value();
  }

  buf = (u_char *)mrb_malloc(mrb, len);

  p = buf;
  for (cl = r->request_body->bufs; cl; cl = cl->next) {
      p = ngx_copy(p, cl->buf->pos, cl->buf->last - cl->buf->pos);
  }

  return mrb_str_new(mrb, (const char *)buf, len);
}

static mrb_value ngx_mrb_get_request_header(mrb_state *mrb, ngx_list_t *headers)
{
  mrb_value mrb_key;
  u_char *key;
  size_t key_len;
  ngx_uint_t i;
  ngx_list_part_t *part;
  ngx_table_elt_t *header;

  mrb_get_args(mrb, "o", &mrb_key);

  key = (u_char *)mrb_str_to_cstr(mrb, mrb_key);
  key_len = RSTRING_LEN(mrb_key);
  part = &headers->part;
  header = part->elts;

  /* TODO:optimize later(linear-search is slow) */
  for (i = 0; /* void */; i++) {
    if (i >= part->nelts) {
      if (part->next == NULL) {
        break;
      }
      part = part->next;
      header = part->elts;
      i = 0;
    }

    if (ngx_strncasecmp(header[i].key.data, key, key_len) == 0) {
      return mrb_str_new(mrb, (const char *)header[i].value.data,
          header[i].value.len);
    }
  }

  return mrb_nil_value();
}

static ngx_int_t ngx_mrb_set_request_header(mrb_state *mrb, ngx_list_t *headers,
    ngx_uint_t assign)
{
  mrb_value mrb_key, mrb_val;
  u_char *key, *val;
  size_t key_len, val_len;
  ngx_uint_t i;
  ngx_list_part_t *part;
  ngx_table_elt_t *header;
  ngx_table_elt_t *new_header;

  mrb_get_args(mrb, "oo", &mrb_key, &mrb_val);

  key = (u_char *)mrb_str_to_cstr(mrb, mrb_key);
  val = (u_char *)mrb_str_to_cstr(mrb, mrb_val);
  key_len = RSTRING_LEN(mrb_key);
  val_len = RSTRING_LEN(mrb_val);
  part  = &headers->part;
  header = part->elts;

  /* TODO:optimize later(linear-search is slow) */
  for (i = 0; /* void */; i++) {
    if (i >= part->nelts) {
      if (part->next == NULL) {
        break;
      }
      part = part->next;
      header = part->elts;
      i = 0;
    }

    if (ngx_strncasecmp(header[i].key.data, key, key_len) == 0) {
      header[i].value.data = val;
      header[i].value.len = val_len;
      return NGX_OK;
    }
  }

  if (assign) {
    new_header = ngx_list_push(headers);
    if (new_header == NULL) {
      return NGX_ERROR;
    }
    new_header->hash = 1;
    new_header->key.data = key;
    new_header->key.len = key_len;
    new_header->value.data = val;
    new_header->value.len = val_len;
  }

  return NGX_OK;
}

static mrb_value ngx_mrb_get_request_headers_in(mrb_state *mrb, mrb_value self)
{
  ngx_http_request_t *r;
  r = ngx_mrb_get_request();
  return ngx_mrb_get_request_header(mrb, &r->headers_in.headers);
}

static mrb_value ngx_mrb_get_request_headers_out(mrb_state *mrb, mrb_value self)
{
  ngx_http_request_t *r;
  r = ngx_mrb_get_request();
  return ngx_mrb_get_request_header(mrb, &r->headers_out.headers);
}

static mrb_value ngx_mrb_set_request_headers_in(mrb_state *mrb, mrb_value self)
{
  ngx_http_request_t *r;
  r = ngx_mrb_get_request();
  ngx_mrb_set_request_header(mrb, &r->headers_in.headers, 0);
  return self;
}

static mrb_value ngx_mrb_set_request_headers_out(mrb_state *mrb, mrb_value self)
{
  ngx_http_request_t *r;
  r = ngx_mrb_get_request();
  ngx_mrb_set_request_header(mrb, &r->headers_out.headers, 1);
  return self;
}

// using from ngx_http_mruby_connection.c and ngx_http_mruby_server.c
mrb_value ngx_mrb_get_request_var(mrb_state *mrb, mrb_value self)
{
  const char *iv_var_str     = "@iv_var";
  mrb_value iv_var;
  struct RClass *class_var, *ngx_class;

  iv_var = mrb_iv_get(mrb, self, mrb_intern_cstr(mrb, iv_var_str));
  if (mrb_nil_p(iv_var)) {
    // get class from Nginx::Var
    ngx_class = mrb_class_get(mrb, "Nginx");
    class_var = (struct RClass*)mrb_class_ptr(mrb_const_get(mrb,
          mrb_obj_value(ngx_class), mrb_intern_cstr(mrb, "Var")));
    // initialize a Var instance
    iv_var = mrb_class_new_instance(mrb, 0, 0, class_var);
    // save Var, avoid multi initialize
    mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, iv_var_str), iv_var);
  }

  return iv_var;
}

static mrb_value ngx_mrb_get_request_var_hostname(mrb_state *mrb,
    mrb_value self)
{
  mrb_value v = ngx_mrb_get_request_var(mrb, self);
  return mrb_funcall(mrb, v, "hostname", 0, NULL);
}

static mrb_value ngx_mrb_get_request_var_filename(mrb_state *mrb,
    mrb_value self)
{
  mrb_value v = ngx_mrb_get_request_var(mrb, self);
  return mrb_funcall(mrb, v, "request_filename", 0, NULL);
}

static mrb_value ngx_mrb_get_request_var_user(mrb_state *mrb, mrb_value self)
{
  mrb_value v = ngx_mrb_get_request_var(mrb, self);
  return mrb_funcall(mrb, v, "remote_user", 0, NULL);
}

// TODO: combine ngx_mrb_get_request_var
static mrb_value ngx_mrb_get_class_obj(mrb_state *mrb, mrb_value self,
    char *obj_id, char *class_name)
{
  mrb_value obj;
  struct RClass *obj_class, *ngx_class;

  obj = mrb_iv_get(mrb, self, mrb_intern_cstr(mrb, obj_id));
  if (mrb_nil_p(obj)) {
    ngx_class = mrb_class_get(mrb, "Nginx");
    obj_class = (struct RClass*)mrb_class_ptr(mrb_const_get(mrb,
          mrb_obj_value(ngx_class), mrb_intern_cstr(mrb, class_name)));
    obj = mrb_obj_new(mrb, obj_class, 0, NULL);
    mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, obj_id), obj);
  }
  return obj;
}

static mrb_value ngx_mrb_headers_in_obj(mrb_state *mrb, mrb_value self)
{
  return ngx_mrb_get_class_obj(mrb, self, "headers_in_obj", "Headers_in");
}

static mrb_value ngx_mrb_headers_out_obj(mrb_state *mrb, mrb_value self)
{
  return ngx_mrb_get_class_obj(mrb, self, "headers_out_obj", "Headers_out");
}

void ngx_mrb_request_class_init(mrb_state *mrb, struct RClass *class)
{
  struct RClass *class_request;
  struct RClass *class_headers_in;
  struct RClass *class_headers_out;

  class_request = mrb_define_class_under(mrb, class, "Request", mrb->object_class);
  mrb_define_method(mrb, class_request, "body", ngx_mrb_get_request_body, MRB_ARGS_NONE());
  mrb_define_method(mrb, class_request, "content_type=", ngx_mrb_set_content_type, MRB_ARGS_ANY());
  mrb_define_method(mrb, class_request, "content_type", ngx_mrb_get_content_type, MRB_ARGS_NONE());
  mrb_define_method(mrb, class_request, "request_line", ngx_mrb_get_request_request_line, MRB_ARGS_NONE());
  mrb_define_method(mrb, class_request, "request_line=", ngx_mrb_set_request_request_line, MRB_ARGS_ANY());
  mrb_define_method(mrb, class_request, "uri", ngx_mrb_get_request_uri, MRB_ARGS_NONE());
  mrb_define_method(mrb, class_request, "uri=", ngx_mrb_set_request_uri, MRB_ARGS_ANY());
  mrb_define_method(mrb, class_request, "unparsed_uri", ngx_mrb_get_request_unparsed_uri, MRB_ARGS_NONE());
  mrb_define_method(mrb, class_request, "unparsed_uri=", ngx_mrb_set_request_unparsed_uri, MRB_ARGS_ANY());
  mrb_define_method(mrb, class_request, "method", ngx_mrb_get_request_method, MRB_ARGS_NONE());
  mrb_define_method(mrb, class_request, "method=", ngx_mrb_set_request_method, MRB_ARGS_ANY());
  mrb_define_method(mrb, class_request, "protocol", ngx_mrb_get_request_protocol, MRB_ARGS_NONE());
  mrb_define_method(mrb, class_request, "protocol=", ngx_mrb_set_request_protocol, MRB_ARGS_ANY());
  mrb_define_method(mrb, class_request, "args", ngx_mrb_get_request_args, MRB_ARGS_NONE());
  mrb_define_method(mrb, class_request, "args=", ngx_mrb_set_request_args, MRB_ARGS_ANY());
  mrb_define_method(mrb, class_request, "var", ngx_mrb_get_request_var, MRB_ARGS_NONE());
  mrb_define_method(mrb, class_request, "headers_in", ngx_mrb_headers_in_obj, MRB_ARGS_NONE());
  mrb_define_method(mrb, class_request, "headers_out", ngx_mrb_headers_out_obj, MRB_ARGS_NONE());

  mrb_define_method(mrb, class_request, "hostname", ngx_mrb_get_request_var_hostname, MRB_ARGS_NONE());
  mrb_define_method(mrb, class_request, "filename", ngx_mrb_get_request_var_filename, MRB_ARGS_NONE());
  mrb_define_method(mrb, class_request, "user", ngx_mrb_get_request_var_user, MRB_ARGS_NONE());

  class_headers_in = mrb_define_class_under(mrb, class, "Headers_in", mrb->object_class);
  mrb_define_method(mrb, class_headers_in, "[]", ngx_mrb_get_request_headers_in, MRB_ARGS_ANY());
  mrb_define_method(mrb, class_headers_in, "[]=", ngx_mrb_set_request_headers_in, MRB_ARGS_ANY());
  mrb_define_method(mrb, class_headers_in, "all", ngx_mrb_get_request_headers_in_hash, MRB_ARGS_ANY());

  class_headers_out = mrb_define_class_under(mrb, class, "Headers_out", mrb->object_class);
  mrb_define_method(mrb, class_headers_out, "[]", ngx_mrb_get_request_headers_out, MRB_ARGS_ANY());
  mrb_define_method(mrb, class_headers_out, "[]=", ngx_mrb_set_request_headers_out, MRB_ARGS_ANY());
  mrb_define_method(mrb, class_headers_out, "all", ngx_mrb_get_request_headers_out_hash, MRB_ARGS_ANY());
}

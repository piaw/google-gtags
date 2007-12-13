// Copyright 2007 Google Inc. All Rights Reserved.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// Author: stephenchen@google.com (Stephen Chen)

#include "mixerrequesthandler.h"

#include "datasource.h"
#include "gtagsmixer.h"
#include "sexpression_util.h"

// Parse command and extract (to our best ability) language and callers setting.
DataSourceRequest* MixerRequestHandler::CreateDataSourceRequest(
    const SExpression* sexpr) const {
  Settings* settings = Settings::instance();
  DataSourceRequest* request = new DataSourceRequest();

  if (!sexpr) {
    request->set_corpus(settings->default_corpus());
    request->set_language(settings->default_language());
    request->set_callers(settings->default_callers());
  }

  const SExpression* corpus_expr = SExpressionAssocGet(sexpr, "corpus");
  if (corpus_expr && corpus_expr->IsString()) {
    request->set_corpus((down_cast<const SExpressionString*>(
                               corpus_expr))->value());
  } else {
    request->set_corpus(settings->default_corpus());
  }

  const SExpression* lang_expr = SExpressionAssocGet(sexpr, "language");
  if (lang_expr && lang_expr->IsString()) {
    request->set_language((down_cast<const SExpressionString*>(
                               lang_expr))->value());
  } else {
    request->set_language(settings->default_language());
  }

  const SExpression* callers_expr = SExpressionAssocGet(sexpr, "callers");
  if (!callers_expr) {
    request->set_callers(settings->default_callers());
  } else {
    request->set_callers(!callers_expr->IsNil());
  }

  // Extract client path information. This information is used by
  // LocalTagsRequestHandler to filter its list of output. Client is the path up
  // until google3. ie. /home/username/src/gtags/google3/tools/tags/gtags.cc's
  // client would be /home/username/src/gtags.
  // Remote gtags servers expect the current-file to be a path relative to
  // google3.
  // If the client sent us fullpath, we strip the prefix here after we extract
  // the client path.
  const SExpression* file_expr = SExpressionAssocGet(sexpr, "current-file");
  if (file_expr && file_expr->IsString()) {
    string filename = (down_cast<const SExpressionString*>(file_expr))->value();
    request->set_client_path(filename);
  } else {
    request->set_client_path("");
  }
  request->set_request(sexpr->Repr());

  return request;
}

bool MixerRequestHandler::IsCommandPing(const SExpression* sexpr) const {
  if (sexpr == NULL || !sexpr->IsList()) {
    return false;
  }

  return sexpr->Begin()->IsSymbol() && sexpr->Begin()->Repr() == "ping";
}

void MixerRequestHandler::Execute(
    const char* command, ResponseCallback* response_callback) const {

  SExpression* sexpr = SExpression::Parse(command);
  // If the client is sending PING, handle it locally in the mixer instead of
  // forwarding to servers.
  if (IsCommandPing(sexpr)) {
    string message = "((value t))";
    delete sexpr;
    Done(response_callback, NULL, message);
    return;
  }

  // request will be deleted in Done.
  DataSourceRequest* request = CreateDataSourceRequest(sexpr);

  delete sexpr;  // Don't need this anymore.
  sexpr = 0;

  const string& corpus = request->corpus();
  DataSourceMap::const_iterator corpus_iter = data_sources_->find(corpus);
  if (corpus_iter == data_sources_->end()) {
    string message = "((error ((message \"Failed to find corpus ";
    message.append(corpus);
    message.append("\"))))");
    Done(response_callback, request, message);
    return;
  }
  const LanguageMap& language_map = corpus_iter->second;

  LanguageMap::const_iterator i = language_map.find(request->language());
  if (i == language_map.end()) {
    string message = "((error ((message \"Failed to map language ";
    message.append(request->language());
    message.append(", callers: ");
    message.append(request->callers() ? "t" : "nil");
    message.append(", corpus: ");
    message.append(request->corpus());
    message.append(" into RPC stubs.\"))))");
    Done(response_callback, request, message);
    return;
  }

  DataSource* source;
  if (request->callers()) {
    source = i->second.second;
  } else {
    source = i->second.first;
  }

  if (!source) {
    string message = "((error ((message \"";
    message.append(request->language());
    message.append(" does not support caller type ");
    message.append(request->callers() ? "t" : "nil");
    message.append("\"))))");
    Done(response_callback, request, message);
    return;
  }

  // Mixer will self destruct on completion of mixing.
  ResultMixer* mixer =
      new ResultMixer(
          NUM_SOURCES_PER_REQUEST,
          gtags::CallbackFactory::Create(
              &MixerRequestHandler::Done, response_callback, request));

  ResultHolder* remote_holder = new ResultHolder(REMOTE, source->size(), mixer);
  source->GetTags(*request, remote_holder);

  LanguageMap::const_iterator local_iterator = language_map.find("local");
  if (local_iterator != language_map.end()) {
    DataSource* local_source;
    if (request->callers()) {
      local_source = local_iterator->second.second;
    } else {
      local_source = local_iterator->second.first;
    }
    ResultHolder* local_holder = new ResultHolder(LOCAL, local_source->size(),
                                                  mixer);
    local_source->GetTags(*request, local_holder);
  } else {
    mixer->set_result("", LOCAL);
  }
}

void MixerRequestHandler::Done(ResponseCallback* response_callback,
                               DataSourceRequest* request,
                               const string& response) {
  response_callback->Run(response);
  delete request;
}

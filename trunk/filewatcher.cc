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

#include "filewatcher.h"

#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#include "pcqueue.h"
#include "strutil.h"

typedef gtags::MutexLock ReaderMutexLock;
typedef gtags::MutexLock WriterMutexLock;

// Join paths together, with '/' added if necessary.
//
// joined_name is the buffer the result will write to. It is assumed to be large
// enough to hold kMaxPathLength characters.
const char* JoinPath(const char* base_path,
                     const char* child_path,
                     char* joined_name) {
  int base_length = strlen(base_path);
  int child_length = strlen(child_path);
  CHECK(base_length + 1 + child_length < kMaxPathLength);

  strncpy(joined_name, base_path, base_length);

  if (joined_name[base_length - 1] != '/') {
    joined_name[base_length] = '/';
    base_length++;
  }

  strncpy(joined_name + base_length, child_path, child_length);
  joined_name[base_length + child_length] = '\0';
  return joined_name;
}

PathnameWatchDescriptorMap::~PathnameWatchDescriptorMap() {
  for (IntCStringMap::iterator iter = wd_to_pathname_.begin();
       iter != wd_to_pathname_.end(); ++iter) {
    delete [] iter->second;
  }
}

void PathnameWatchDescriptorMap::Add(const char* pathname, int wd) {
  // Make a copy of pathname
  int length = strlen(pathname);
  char* pathname_cpy = new char[length + 1];
  strncpy(pathname_cpy, pathname, length);
  pathname_cpy[length] = '\0';

  WriterMutexLock lock(&mutex_);
  pathname_to_wd_map_[pathname_cpy] = wd;
  wd_to_pathname_[wd] = pathname_cpy;
}

void PathnameWatchDescriptorMap::Remove(int wd) {
  WriterMutexLock lock(&mutex_);
  const char* pathname = GetPathnameNoLock(wd);
  if (pathname == NULL) {
    // We don't have the pathname in our wd-to-pathname mapping.
    return;
  }

  LOG(INFO) << "Removing " << pathname;
  pathname_to_wd_map_.erase(pathname);
  wd_to_pathname_.erase(wd);
  delete [] pathname;
}

int PathnameWatchDescriptorMap::GetWatchDescriptor(const char* pathname) {
  ReaderMutexLock lock(&mutex_);
  CStringIntMap::iterator iter = pathname_to_wd_map_.find(pathname);
  if (iter != pathname_to_wd_map_.end()) {
    return iter->second;
  }
  return 0;
}

const char* PathnameWatchDescriptorMap::GetPathname(int wd) {
  ReaderMutexLock lock(&mutex_);
  return GetPathnameNoLock(wd);
}

const char* PathnameWatchDescriptorMap::GetPathnameNoLock(int wd) {
  IntCStringMap::iterator iter = wd_to_pathname_.find(wd);
  if (iter != wd_to_pathname_.end()) {
    return iter->second;
  }
  return NULL;
}

void PathnameWatchDescriptorMap::GetSubDirs(const char* pathname,
                               list<const char*>* output) {
  ReaderMutexLock lock(&mutex_);
  for (CStringIntMap::iterator iter = pathname_to_wd_map_.begin();
       iter != pathname_to_wd_map_.end(); ++iter) {
    if (var_strprefix(iter->first, pathname)) {
      output->push_back(iter->first);
    }
  }
}

void PathnameWatchDescriptorMap::GetSubDirsWatchDescriptor(const char* pathname,
                                 list<int>* output) {
  ReaderMutexLock lock(&mutex_);
  for (CStringIntMap::iterator iter = pathname_to_wd_map_.begin();
       iter != pathname_to_wd_map_.end(); ++iter) {
    if (var_strprefix(iter->first, pathname)) {
      output->push_back(iter->second);
    }
  }
}

void InotifyEventHandler::HandleEvent(struct inotify_event* event) {
  // Check against all filters before we handle the event.
  for (list<InotifyEventFilter*>::const_iterator iter = filters_.begin();
       iter != filters_.end(); ++iter) {
    if ((*iter)->DoFilter(event) == false) {
      return;
    }
  }

  if (event->mask & IN_ACCESS) {
    HandleAccess(event);
  } else if (event->mask & IN_MODIFY) {
    HandleModify(event);
  } else if (event->mask & IN_ATTRIB) {
    HandleAttrib(event);
  } else if (event->mask & IN_CLOSE_WRITE) {
    HandleCloseWrite(event);
  } else if (event->mask & IN_CLOSE_NOWRITE) {
    HandleCloseNoWrite(event);
  } else if (event->mask & IN_OPEN) {
    HandleOpen(event);
  } else if (event->mask & IN_MOVED_FROM) {
    HandleMovedFrom(event);
  } else if (event->mask & IN_MOVED_TO) {
    HandleMovedTo(event);
  } else if (event->mask & IN_CREATE) {
    HandleCreate(event);
  } else if (event->mask & IN_DELETE) {
    HandleDelete(event);
  } else {
    LOG(INFO) << "Unhandled inotify event: mask=" << event->mask << " name="
              << event->name << "\n";
  }
}

void InotifyFileWatcher::Loop() {
  Init();
  while(true) {
    ProcessEvents();
  }
}

void InotifyFileWatcher::Init() {
  fd_ = inotify_init();
  CHECK(fd_ >= 0) << "Inotify failed to initialize.";
}

void DirectoryTracker::HandleCreate(struct inotify_event* event) {
  const char* base_path = watcher_->GetPathname(event->wd);
  CHECK(base_path != NULL) << "base directory not known for watch descriptor "
                           << event->wd;

  const char* full_path = JoinPath(base_path, event->name, join_name_buffer_);
  watcher_->AddDirectory(full_path);
}

void DirectoryTracker::HandleDelete(struct inotify_event* event) {
  const char* base_path = watcher_->GetPathname(event->wd);
  CHECK(base_path != NULL) << "base directory not known for watch descriptor "
                           << event->wd;

  const char* full_path = JoinPath(base_path, event->name, join_name_buffer_);

  int wd = watcher_->GetWatchDescriptor(full_path);
  CHECK(wd > 0);
  watcher_->RemoveDirectory(wd);
}

int InotifyFileWatcher::AddDirectory(const char* dir_name) {
  // Check if we are already watching the dir
  int wd = map_.GetWatchDescriptor(dir_name);
  if (wd > 0) {
    return wd;
  }

  wd = AddInotifyWatch(dir_name);
  LOG(INFO) << "Watching: " << dir_name;
  CHECK(wd >= 0);
  map_.Add(dir_name, wd);
  return wd;
}

void InotifyFileWatcher::AddDirectoryRecursive(const char* dir_name) {
  DIR* dir = opendir(dir_name);
  if (!dir) {
    return;
  }

  struct dirent* content;
  struct stat stats;
  while ((content = readdir(dir)) != NULL) {
    if (strcmp(content->d_name, ".") == 0) {
      continue;
    }

    if (strcmp(content->d_name, "..") == 0) {
      continue;
    }

    if (exclude_list_.find(content->d_name) != exclude_list_.end()) {
      LOG(INFO) << "Excluding " << content->d_name;
      continue;
    }

    string child_path =
        JoinPath(dir_name, content->d_name, join_name_buffer_);
    // Don't follow symlink.
    if (lstat(child_path.c_str(), &stats) == 0 && !S_ISLNK(stats.st_mode)) {
      if (S_ISDIR(stats.st_mode)) {
        AddDirectoryRecursive(child_path.c_str());
      } else {
        list<InotifyEventHandler*>::iterator iter;
        for (iter = event_handlers_.begin(); iter != event_handlers_.end();
             ++iter) {
          (*iter)->HandleImport(child_path.c_str());
        }
      }
    }
  }
  closedir(dir);
  AddDirectory(dir_name);
}

void InotifyFileWatcher::RemoveDirectoryRecursive(const char* dirname) {
  LOG(INFO) << "Removing: " << dirname;
  int wd = map_.GetWatchDescriptor(dirname);
  RemoveDirectory(wd);

  list<int> dirs;
  map_.GetSubDirsWatchDescriptor(dirname, &dirs);
  for (list<int>::iterator i = dirs.begin(); i != dirs.end(); ++i) {
    RemoveDirectory(*i);
  }
}

void InotifyFileWatcher::RemoveDirectory(int wd) {
  RemoveInotifyWatch(wd);
  map_.Remove(wd);
}


void InotifyFileWatcher::AddExcludeDirectory(const char* dir) {
  int length = strlen(dir);
  if (length > 1 && dir[length - 1] == '/') {  // Don't include ending /.
    length--;
  }

  char* exclude_dir = new char[length + 1];
  strncpy(exclude_dir, dir, length);
  exclude_dir[length] = '\0';

  if (exclude_list_.find(exclude_dir) != exclude_list_.end()) {
    LOG(INFO) << exclude_dir << " already in exclude list.";
    delete [] exclude_dir;  // already have it.
  } else {
    LOG(INFO) << "Adding " << exclude_dir << " to exclude list.";
    exclude_list_.insert(exclude_dir);
  }
}

void InotifyFileWatcher::RemoveExcludeDirectory(const char* dir) {
  int length = strlen(dir);
  if (length > 1 && dir[length - 1] == '/') {  // Don't include ending /.
    length--;
  }

  char* exclude_dir = new char[length + 1];
  strncpy(exclude_dir, dir, length);
  exclude_dir[length] = '\0';

  hash_set<const char*, hash<const char*>, StrEq>::iterator iter;
  iter = exclude_list_.find(exclude_dir);

  if ( iter != exclude_list_.end()) {
    const char* tmp = *iter;
    LOG(INFO) << "Removing " << tmp << " from exclude list.";
    exclude_list_.erase(iter);
    delete[] tmp;
  }
  delete [] exclude_dir;  // dont have it.
}

void InotifyFileWatcher::ProcessEvents() {
  // Read inotify events. Block if there are no events to be read.
  int len = ReadEvents();
  if (len < 0) {
    if (errno == EINTR) {  // System call interrupted by signal.
      return;              // Return and let caller retry.
    } else {
      LOG(FATAL) << "Inotify failed to read. Errno: " << errno;
    }
  }

  if (!len) {
    LOG(INFO) << "Buffer size too small.";
    return;
  }

  int offset = 0;
  while (offset < len) {
    // Internally, the buffer looks like:
    // |struct inotify_event1|filename1|struct inotify_event2|filename2...|
    // To read an inotify_event, cast event_buffer_ to a struct inotify_event*
    // To advance to the next event, increment event_buffer_ by
    // sizeof(struct inotify_event) + current_inotify_event_instance.len.
    struct inotify_event* event =
        reinterpret_cast<struct inotify_event*>(event_buffer_ + offset);

    for (list<InotifyEventHandler*>::iterator iter = event_handlers_.begin();
         iter != event_handlers_.end(); ++iter) {
      (*iter)->HandleEvent(event);
    }
    offset += kInotifyEventSize + event->len;
  };
}

void IndexEventHandler::HandleImport(const char* filename) {
  // Check against all filters before we handle the event.
  for (list<InotifyEventFilter*>::const_iterator iter = filters_.begin();
       iter != filters_.end(); ++iter) {
    if ((*iter)->DoFilterOnFilename(filename) == false) {
      return;
    }
  }

  // Make copy of filename and add to index request queue.
  // Filename will be deallocated on the receiving end.
  int length = strlen(filename);
  char* filename_cpy = new char[length + 1];
  strncpy(filename_cpy, filename, length);
  filename_cpy[length] = '\0';
  queue_->Put(filename_cpy);
}

// Copy the full path to the changed file to a buffer and add to the PCQueue.
void IndexEventHandler::DoIndexing(struct inotify_event* event) {
  const char* base_path = watcher_->GetPathname(event->wd);
  if (!base_path) {
    LOG(WARNING) << "unknown watch descriptor: " << event->wd;
    return;
  }

  // full_path will be destructed in the receiving end of the PCQueue.
  // size is parent dir length + / + filename length + \0
  char* full_path = new char[strlen(base_path) + event->len + 2];
  JoinPath(base_path, event->name, full_path);
  queue_->Put(full_path);
}

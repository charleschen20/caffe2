// This binary provides an easy way to open a zeromq server and feeds data to
// clients connect to it. It uses the Caffe2 db as the backend, thus allowing
// one to convert any db-compliant storage to a zeromq service.

#include <atomic>

#include "caffe2/core/db.h"
#include "caffe2/core/init.h"
#include "caffe2/core/logging.h"
#include "caffe2/utils/zmq_helper.h"

CAFFE2_DEFINE_string(server, "tcp://*:5555", "The server address.");
CAFFE2_DEFINE_string(input_db, "", "The input db.");
CAFFE2_DEFINE_string(input_db_type, "", "The input db type.");

using caffe2::db::DB;
using caffe2::db::Cursor;
using caffe2::string;

std::unique_ptr<DB> in_db;
std::unique_ptr<Cursor> cursor;

int main(int argc, char** argv) {
  caffe2::GlobalInit(&argc, argv);

  CAFFE_LOG_INFO << "Opening DB...";
  in_db.reset(caffe2::db::CreateDB(
      caffe2::FLAGS_input_db_type, caffe2::FLAGS_input_db, caffe2::db::READ));
  CAFFE_CHECK(in_db.get() != nullptr) << "Cannot load input db.";
  cursor.reset(in_db->NewCursor());
  CAFFE_LOG_INFO << "DB opened.";

  CAFFE_LOG_INFO << "Starting ZeroMQ server...";

  //  Socket to talk to clients
  caffe2::ZmqSocket sender(ZMQ_PUSH);
  sender.Bind(caffe2::FLAGS_server);
  CAFFE_LOG_INFO << "Server created at " << caffe2::FLAGS_server;

  while (1) {
    CAFFE_VLOG(1) << "Sending " << cursor->key();
    sender.SendTillSuccess(cursor->key(), ZMQ_SNDMORE);
    sender.SendTillSuccess(cursor->value(), 0);
    cursor->Next();
    if (!cursor->Valid()) {
      cursor->SeekToFirst();
    }
  }
  // We do not do an elegant quit since this binary is going to be terminated by
  // control+C.
  return 0;
}

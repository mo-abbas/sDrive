if exist "src/proto/caffe.pb.h" (
    echo caffe.pb.h remains the same as before
) else (
    echo caffe.pb.h is being generated
    "../3rdparty/bin/protoc" -I="src/caffe/proto" --cpp_out="src/proto" --python_out="../python/proto" "src/proto/caffe.proto"
    "../3rdparty/bin/protoc" -I="src/caffe/proto" --cpp_out="../include/caffe/proto" "src/proto/caffe.proto"
)

::if exist "../src/caffe/proto/caffe_pretty_print.pb.h" (
::    echo caffe_pretty_print.pb.h remains the same as before
::) else (
::    echo caffe_pretty_print.pb.h is being generated
::    "../3rdparty/bin/protoc" -I="../src/caffe/proto" --cpp_out="../src/caffe/proto" "../src/caffe/proto/caffe_pretty_print.proto"
::)



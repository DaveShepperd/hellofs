#!/bin/bash

set -e

root_pwd="$PWD"
test_dir="test-dir-$RANDOM"
test_mount_point="test-mount-point-$RANDOM"

function create_test_image() {
    dd bs=4096 count=1028 if=/dev/zero of="$1"
    ./mkfs-hellofs "$1"
}

function mount_fs_image() {
    sudo insmod ./hellofs.ko
#    sudo mount -o loop,owner,group,users -t hellofs "$1" "$2"
    sudo mount -o loop -t hellofs "$1" "$2"
    sudo chmod 777 $2
}

function unmount_fs() {
    sudo umount "$1"
    sudo rmmod ./hellofs.ko
}

function do_some_operations() {
    cd "$1"

    ls
    cat wel_helo.txt

    cp wel_helo.txt hello
    cat hello

    echo "Hello World" > hello
    cat hello

    mkdir dir1 && cd dir1

    cp ../hello .
    cat hello

    echo "First level directory" > hello
    cat hello

    mkdir dir2 && cd dir2

    touch hello
    cat hello

    echo "Second level directory" > hello
    cat hello

    cp hello hello_smaller
    echo "smaller" > hello_smaller
    cat hello_smaller
}

function do_read_operations()
{
    cd "$1"
    ls -lR

    cat wel_helo.txt
    cat hello

    cat hello

    cd dir1
    cat hello

    cd dir2
    cat hello
    cat hello_smaller
}

function cleanup() {
    cd "$root_pwd"
    sudo mount | grep -q "$test_mount_point" && sudo umount -t hellofs "$test_mount_point"
    lsmod | grep -q hellofs && sudo rmmod "$root_pwd/hellofs.ko"
    rm -fR "$test_dir" "$test_mount_point"
}

set -x
make

cleanup
trap cleanup SIGINT EXIT
mkdir "$test_dir" "$test_mount_point"
create_test_image "$test_dir/image"

# run 1
mount_fs_image "$test_dir/image" "$test_mount_point"
do_some_operations "$test_mount_point"
cd "$root_pwd"
unmount_fs "$test_mount_point"

# run 2
mount_fs_image "$test_dir/image" "$test_mount_point"
do_read_operations "$test_mount_point"
cd "$root_pwd"
ls -lR "$test_mount_point"
unmount_fs "$test_mount_point"

echo "Test finished successfully!"
cleanup

make clean
rm -rf "$test_dir/image" "$test_mount_point"

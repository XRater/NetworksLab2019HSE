#include <utility>

#include "Task.h"

#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <regex>
#include <fstream>
#include <iostream>

Task::Task(int socket, std::string root_directory) {
    this->socket = socket;
    this->directory = root_directory;
    this->root_directory = root_directory;
}

void Task::cd_in_dir() {
    std::regex short_path(".*\\/..");
    std::string path = get_string();
    std::string new_path = std::regex_replace(this->directory + path, short_path, "");
    if (!(new_path.length() < root_directory.length() || !strncmp(new_path.c_str(), root_directory.c_str(), root_directory.length()))) {
        directory = new_path;
        int answer = CD_IN_DIR_SUCC;
        write(socket, &answer, sizeof(int));
        send_string(new_path);
    } else {
        int answer = CD_IN_DIR_FAIL;
        send_num(answer);
        send_string(directory);
    }
}

void Task::get_file_list() {
    int zero = get_num();
    std::vector<std::string> file_list = get_file_list_in_dir();
    int message_length = sizeof(int);
    for (auto &i : file_list) {
        message_length += i.length() + sizeof(int);
    }
    int file_number = file_list.size();
    int answer = GET_FILE_LIST_SUCC;
    send_num(answer);
    send_num(message_length);
    send_num(file_number);
    for (auto &i : file_list) {
        send_string(i);
    }
}

void Task::get_file() {
    int message_size = get_num();
    std::string file_name = get_string();
    FILE *fp;
    if((fp = fopen(file_name.c_str(), "w+b")) == NULL) {
        int answer = GET_FILE_FAIL;
        send_num(answer);
        send_string(file_name);
        return;
    }
    int size;
    read(socket, &size, sizeof(int));
    char* buf = new char[size];
    read(socket, buf, size);
    fwrite (buf , sizeof(char), size, fp);
    delete[] buf;
    fclose(fp);
    int answer = GET_FILE_SUCC;
    send_num(answer);
    send_string(file_name.c_str());
}

void Task::send_file() {
    std::string file_name = get_string();
    FILE *fp;
    if((fp = fopen(file_name.c_str(), "rb")) == NULL) {
        send_num(SEND_FILE_FAIL);
        send_string(file_name);
        return;
    }
    fseek(fp , 0 , SEEK_END);
    int file_size = ftell(fp);
    rewind(fp);
    char* buf = new char[file_size];
    fread (buf, sizeof(char), file_size, fp);
    send_num(SEND_FILE_SUCC);
    send_num(file_size);
    write(socket, buf, file_size);
    delete[] buf;
}

void Task::send_string(std::string string_to_send) {
    int length = string_to_send.length();
    send_num(length);
    write(socket, string_to_send.c_str(), length);
}

std::string Task::get_string() {
    int length = get_num();
    char* buf = new char[length + 1];
    read(socket, buf, length);
    buf[length] = '\0';
    std::string ans(buf);
    delete[] buf;
    return ans;
}

std::vector<std::string> Task::get_file_list_in_dir() {
    std::vector<std::string> result;
    struct dirent *entry;
    DIR *dir = opendir(directory.c_str());

    if (dir == NULL) {
        return result;
    }

    while ((entry = readdir(dir)) != NULL) {
        result.emplace_back(entry->d_name);
    }
    closedir(dir);
    return result;
}

int Task::get_num() {
    int num;
    read(socket, &num, sizeof(int));
    return ntohl(num);
}

void Task::send_num(int num) {
    num = htonl(num);
    write(socket, &num, sizeof(int));
}

void clientWork(int socket, std::string root_dir) {
    std::cout << "hey";
    Task task = Task(socket, std::move(root_dir));
    while (true) {
        int command = task.get_num();
        switch (command) {
            case CD_IN_DIR:
                task.cd_in_dir();
                break;
            case GET_FILE_LIST:
                task.get_file_list();
                break;
            case GET_FILE:
                task.get_file();
                break;
            case SEND_FILE:
                task.send_file();
                break;
            default:
                return;
        }
    }
}
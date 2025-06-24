#include <iostream>
#include <queue>
#include <deque>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

int ValidateSymLink(std::string path)
{
    struct stat buf;

    int res = lstat (path.c_str(), &buf);

    if (res != 0)
    {
        std::cout << "Target does not exist." << std::endl;
        return 0;
    }
    
    if (S_ISLNK(buf.st_mode))
    {
        return 1;
    }

    return 0;
}

std::string GetCanonicalPath(std::string path)
{
    char buf[PATH_MAX];
    char *res = realpath(path.c_str(), buf);

    if (res)
    {
        return std::string(buf);
    }

    return path;
}

int CreateSymLink(std::string source, std::string destination)
{
    auto canonical = GetCanonicalPath(source);

    if (canonical == "")
    {
        return 0;
    }

    symlink(canonical.c_str(), destination.c_str());

    return 1;
}

int Remove(std::string path)
{
    struct stat buf;
    int res = lstat (path.c_str(), &buf);

    if (res != 0)
    {
        printf("Target does not exist.\n");
        return -1;
    }
    else if (S_ISLNK(buf.st_mode) || !S_ISDIR(buf.st_mode))
    {
        unlink(path.c_str());
        return 1;
    }

    std::queue<std::string> dirsToCheck;
    dirsToCheck.push(path);
    std::deque<std::string> dirsToRemove;
    dirsToRemove.push_front(path);

    while (!dirsToCheck.empty())
    {
        auto dirPath = dirsToCheck.front();
        dirsToCheck.pop();

        struct dirent *ent;
        DIR *dir = opendir (dirPath.c_str());
        if (dir == NULL)
        {
            std::cout << "Could not open: " << dirPath << std::endl;
            continue;
        }

        while ((ent = readdir (dir)) != NULL)
        {
            auto entPath = std::string(ent->d_name);
            if (entPath == "." || entPath == "..")
            {
                continue;
            }

            entPath = dirPath + "/" + entPath;
            res = lstat (entPath.c_str(), &buf);
            if (res != 0)
            {
                continue;
            }
            else if (S_ISLNK(buf.st_mode) || !S_ISDIR(buf.st_mode))
            {
                unlink(entPath.c_str());
            }
            else if (S_ISDIR(buf.st_mode))
            {
                dirsToCheck.push(entPath);
                dirsToRemove.push_front(entPath);
            }
        }

        closedir (dir);
    }

    while (!dirsToRemove.empty())
    {
        auto dirPath = dirsToRemove.front();
        dirsToRemove.pop_front();

        rmdir(dirPath.c_str());
    }

    return 1;
}

int Scan(std::string path)
{
    struct stat buf;
    int res = lstat (path.c_str(), &buf);

    if (res != 0)
    {
        printf("Target does not exist.\n");
        return 0;
    }
    else if (S_ISLNK(buf.st_mode))
    {
        return 1;
    }
    else if (!S_ISDIR(buf.st_mode))
    {
        std::cout << "Target is regular file" << std::endl;
        return 0;
    }

    std::queue<std::string> dirsToCheck;
    dirsToCheck.push(path);

    while (!dirsToCheck.empty())
    {
        auto dirPath = dirsToCheck.front();
        dirsToCheck.pop();

        struct dirent *ent;
        DIR *dir = opendir (dirPath.c_str());
        if (dir == NULL)
        {
            std::cout << "Could not open: " << dirPath << std::endl;
            continue;
        }

        while ((ent = readdir (dir)) != NULL)
        {
            auto entPath = std::string(ent->d_name);
            if (entPath == "." || entPath == "..")
            {
                continue;
            }

            entPath = dirPath + "/" + entPath;
            res = lstat (entPath.c_str(), &buf);
            if (res != 0)
            {
                continue;
            }
            else if (S_ISLNK(buf.st_mode))
            {
                std::cout << "Found symlink: " << entPath << std::endl;
                return 1;
            }
            else if (S_ISDIR(buf.st_mode))
            {
                dirsToCheck.push(entPath);
            }
        }

        closedir (dir);
    }

    std::cout << "Symlink not found" << std::endl;

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cout << "Missing arguments." << std::endl;
        return -1;
    }

    auto option = std::string(argv[1]);
    auto path = std::string(argv[2]);

    if (path.back() == '/' || path.back() == '\\')
    {
        path.pop_back();
    }

    if (path.length() >= 2 && path[1] == ':')
    {
        std::cout << "Path is in windows format." << std::endl;
        return -1;
    }

    if (option == "-c" || option == "-C")
    {
        if (argc < 4)
        {
            std::cout << "Missing destination argument." << std::endl;
            return -1;
        }

        auto destination = std::string(argv[3]);

        if (destination.back() == '/' || destination.back() == '\\')
        {
            destination.pop_back();
        }

        return CreateSymLink(path, destination);
    }
    else if (option == "-v" || option == "-V")
    {
        return ValidateSymLink(path);
    }
    else if (option == "-r" || option == "-R")
    {
        return Remove(path);
    }
    else if (option == "-s" || option == "-S")
    {
        return Scan(path);
    }

    std::cout << "Invalid option." << std::endl;

    return -1;
}

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <filesystem>
#include <limits>

struct Usuario {
    std::string username;
    std::string password;
};

struct Carro {
    std::string modelo;
    int ano = 0;
    double precoPorDia = 0.0;
    bool disponivel = true;
};

static const std::string USERS_FILE = "users.json";
static const std::string CARS_FILE = "cars.json";

bool fileExists(const std::string &path) {
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

std::string readAll(const std::string &path) {
    std::ifstream in(path, std::ios::in);
    if (!in) return {};
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return content;
}

void writeAll(const std::string &path, const std::string &content) {
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    out << content;
}

std::vector<Usuario> loadUsuarios() {
    std::vector<Usuario> users;
    if (!fileExists(USERS_FILE)) {
        // cria padrão
        users.push_back({"admin", "admin"});
        return users;
    }
    std::string json = readAll(USERS_FILE);
    if (json.find("users") == std::string::npos) {
        users.push_back({"admin", "admin"});
        return users;
    }
    std::regex userRe("\\{\\s*\"username\"\\s*:\\s*\"([^\"]+)\"\\s*,\\s*\"password\"\\s*:\\s*\"([^\"]+)\"\\s*\\}");
    auto begin = std::sregex_iterator(json.begin(), json.end(), userRe);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        Usuario u{(*it)[1], (*it)[2]};
        users.push_back(u);
    }
    if (users.empty()) {
        users.push_back({"admin", "admin"});
    }
    return users;
}

void saveUsuarios(const std::vector<Usuario> &users) {
    std::string out = "{\n  \"users\": [\n";
    for (size_t i = 0; i < users.size(); ++i) {
        out += "    {\"username\": \"" + users[i].username + "\", \"password\": \"" + users[i].password + "\"}";
        if (i + 1 < users.size()) out += ",";
        out += "\n";
    }
    out += "  ]\n}";
    writeAll(USERS_FILE, out);
}

std::vector<Carro> loadCarros() {
    std::vector<Carro> cars;
    if (!fileExists(CARS_FILE)) {
        return cars; // vazio
    }
    std::string json = readAll(CARS_FILE);
    if (json.find("cars") == std::string::npos) return cars;

    std::regex carRe(
        "\\{\\s*\"modelo\"\\s*:\\s*\"([^\"]+)\"\\s*,\\s*\"ano\"\\s*:\\s*([0-9]+)\\s*,\\s*\"precoPorDia\"\\s*:\\s*([0-9]+(?:\\.[0-9]+)?)\\s*,\\s*\"disponivel\"\\s*:\\s*(true|false)\\s*\\}");
    auto begin = std::sregex_iterator(json.begin(), json.end(), carRe);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        Carro c;
        c.modelo = (*it)[1];
        c.ano = std::stoi((*it)[2]);
        c.precoPorDia = std::stod((*it)[3]);
        c.disponivel = ((*it)[4] == "true");
        cars.push_back(c);
    }
    return cars;
}

void saveCarros(const std::vector<Carro> &cars) {
    std::string out = "{\n  \"cars\": [\n";
    for (size_t i = 0; i < cars.size(); ++i) {
        const auto &c = cars[i];
        out += "    {\"modelo\": \"" + c.modelo + "\", \"ano\": " + std::to_string(c.ano)
               + ", \"precoPorDia\": " + std::to_string(c.precoPorDia)
               + ", \"disponivel\": " + std::string(c.disponivel ? "true" : "false") + "}";
        if (i + 1 < cars.size()) out += ",";
        out += "\n";
    }
    out += "  ]\n}";
    writeAll(CARS_FILE, out);
}

void ensureDataFiles() {
    if (!fileExists(USERS_FILE)) {
        std::vector<Usuario> defaults = {{"admin", "admin"}};
        saveUsuarios(defaults);
    }
    if (!fileExists(CARS_FILE)) {
        std::vector<Carro> empty;
        saveCarros(empty);
    }
}

bool realizarLogin(const std::vector<Usuario> &users) {
    std::cout << "=== Login ===\n";
    for (int tent = 0; tent < 3; ++tent) {
        std::string u, p;
        std::cout << "Usuário: ";
        std::getline(std::cin, u);
        std::cout << "Senha: ";
        std::getline(std::cin, p);
        for (const auto &user : users) {
            if (user.username == u && user.password == p) {
                std::cout << "Login bem-sucedido!\n";
                return true;
            }
        }
        std::cout << "Credenciais inválidas. Tente novamente.\n";
    }
    std::cout << "Número máximo de tentativas atingido.\n";
    return false;
}

void cadastrarCarro(std::vector<Carro> &cars) {
    std::cout << "=== Cadastrar Carro ===\n";
    Carro c;
    std::cout << "Modelo: ";
    std::getline(std::cin, c.modelo);
    std::cout << "Ano: ";
    while (!(std::cin >> c.ano)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Entrada inválida. Informe um ano válido: ";
    }
    std::cout << "Preço por dia: ";
    while (!(std::cin >> c.precoPorDia)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Entrada inválida. Informe um preço válido: ";
    }
    std::cout << "Disponível (s/n): ";
    char opc;
    std::cin >> opc;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // limpar fim de linha
    c.disponivel = (opc == 's' || opc == 'S');

    cars.push_back(c);
    saveCarros(cars);
    std::cout << "Carro cadastrado com sucesso!\n";
}

void listarDisponiveis(const std::vector<Carro> &cars) {
    std::cout << "=== Carros Disponíveis ===\n";
    int count = 0;
    for (const auto &c : cars) {
        if (c.disponivel) {
            std::cout << "- Modelo: " << c.modelo
                      << ", Ano: " << c.ano
                      << ", Preço/dia: R$ " << c.precoPorDia
                      << ", Disponível: " << (c.disponivel ? "Sim" : "Não")
                      << "\n";
            ++count;
        }
    }
    if (count == 0) {
        std::cout << "Nenhum carro disponível no momento." << std::endl;
    }
}

int main() {
    ensureDataFiles();

    auto usuarios = loadUsuarios();
    auto carros = loadCarros();

    // Garantir que std::getline funcione após possíveis leituras anteriores
    std::cin.clear();

    if (!realizarLogin(usuarios)) {
        return 0;
    }

    while (true) {
        std::cout << "\n=== Menu ===\n";
        std::cout << "1. Cadastrar Carro\n";
        std::cout << "2. Listar Carros Disponíveis\n";
        std::cout << "3. Sair\n";
        std::cout << "Escolha uma opção: ";
        int op = 0;
        if (!(std::cin >> op)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Opção inválida.\n";
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // consumir fim de linha

        if (op == 1) {
            cadastrarCarro(carros);
        } else if (op == 2) {
            listarDisponiveis(carros);
        } else if (op == 3) {
            std::cout << "Saindo...\n";
            break;
        } else {
            std::cout << "Opção não reconhecida.\n";
        }
    }

    return 0;
}
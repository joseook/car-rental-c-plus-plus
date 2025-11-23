#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <filesystem>
#include <limits>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <cstdio>
#include "../Includes/nlohmann/json.hpp"


using json = nlohmann::json;

struct Usuario {
    std::string username;
    std::string password;
};

struct Carro {
    std::string modelo;
    int ano = 0;
    double precoPorDia = 0.0; // pq diabos tu colocou preço dia no carro? e pq esse caralhos ta em double ??
    bool disponivel = true;
};

struct Aluguel {
    std::string modelo;
    std::string dataInicio;
    std::string dataFim;
    int dias;
    double precoPorDia; // usando double na fé, copiando a ideia de maluco da struc acima 
    double total;
    bool devolvido; 
    std::string usuario;
};

static const std::string USERS_FILE = "users.json";
static const std::string CARS_FILE = "cars.json";
static const std::string RENTALS_FILE = "rentals.json";

bool fileExists(const std::string& path) {
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

std::string readAll(const std::string& path) {
    std::ifstream in(path, std::ios::in);
    if (!in) return {};
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return content;
}

void writeAll(const std::string& path, const std::string& content) {
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    out << content;
}

std::vector<Usuario> loadUsuarios() {
    std::vector<Usuario> users;
    if (!fileExists(USERS_FILE)) {
        // cria padrão
        users.push_back({ "admin", "admin" });
        return users;
    }
    std::string json = readAll(USERS_FILE);
    if (json.find("users") == std::string::npos) {
        users.push_back({ "admin", "admin" });
        return users;
    }
	// cara ta achando que ta no javascript kkkkkkkkkkkkkkkkkkk
    std::regex userRe("\\{\\s*\"username\"\\s*:\\s*\"([^\"]+)\"\\s*,\\s*\"password\"\\s*:\\s*\"([^\"]+)\"\\s*\\}");
    auto begin = std::sregex_iterator(json.begin(), json.end(), userRe);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        Usuario u{ (*it)[1], (*it)[2] };
        users.push_back(u);
    }
    if (users.empty()) {
        users.push_back({ "admin", "admin" });
    }
    return users;
}

void saveUsuarios(const std::vector<Usuario>& users) {
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

void saveCarros(const std::vector<Carro>& cars) {
    std::string out = "{\n  \"cars\": [\n";
    for (size_t i = 0; i < cars.size(); ++i) {
        const auto& c = cars[i];
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
        std::vector<Usuario> defaults = { {"admin", "admin"} };
        saveUsuarios(defaults);
    }
    if (!fileExists(CARS_FILE)) {
        std::vector<Carro> empty;
        saveCarros(empty);
    }
    if (!fileExists(RENTALS_FILE)) {
        writeAll(RENTALS_FILE, "{\n  \"rentals\": [\n  ]\n}");
    }
}

std::string ObterDataAtual() {
    auto now = std::time(nullptr);
    std::tm tm;
    localtime_s(&tm, &now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
}

int CalcularDias(const std::string& dataInicio, const std::string& dataFim) {
    std::tm tm1 = {}, tm2 = {};
    std::istringstream iss1(dataInicio);
    std::istringstream iss2(dataFim);
    iss1 >> std::get_time(&tm1, "%Y-%m-%d");
    iss2 >> std::get_time(&tm2, "%Y-%m-%d");
    
    if (iss1.fail() || iss2.fail()) return 1;
    
    auto time1 = std::mktime(&tm1);
    auto time2 = std::mktime(&tm2);
    double diff = std::difftime(time2, time1) / (60 * 60 * 24);
    return static_cast<int>(diff) + 1; // +  1 para por o dia inicial 
}

std::vector<Aluguel> CarregarAlugueis() {
    std::vector<Aluguel> alugueis;
    if (!fileExists(RENTALS_FILE)) {
        return alugueis;
    }
    
    try {
        std::ifstream file(RENTALS_FILE);
        if (!file.is_open()) {
            return alugueis;
        }
        
        json j;
        file >> j;
        file.close();
        
        if (j.contains("rentals") && j["rentals"].is_array()) {
            for (const auto& item : j["rentals"]) {
                Aluguel a;
                a.modelo = item.value("modelo", "");
                a.dataInicio = item.value("dataInicio", "");
                a.dataFim = item.value("dataFim", "");
                a.dias = item.value("dias", 0);
                a.precoPorDia = item.value("precoPorDia", 0.0);
                a.total = item.value("total", 0.0);
                a.devolvido = item.value("devolvido", false);
                a.usuario = item.value("usuario", "");
                alugueis.push_back(a);
            }
        }
    } catch (const std::exception& e) {
      
    }
    
    return alugueis;
}

void SalvarAlugueis(const std::vector<Aluguel>& alugueis) {
    json j;
    j["rentals"] = json::array();
    
    for (const auto& a : alugueis) {
        json rental;                              // olha só como é mais organizado usando JSON
        rental["modelo"] = a.modelo;              // olha só como é mais organizado usando JSON
        rental["dataInicio"] = a.dataInicio;      // olha só como é mais organizado usando JSON
        rental["dataFim"] = a.dataFim;            // olha só como é mais organizado usando JSON
        rental["dias"] = a.dias;                  // olha só como é mais organizado usando JSON
        rental["precoPorDia"] = a.precoPorDia;    // olha só como é mais organizado usando JSON
        rental["total"] = a.total;                // olha só como é mais organizado usando JSON
        rental["devolvido"] = a.devolvido;        // olha só como é mais organizado usando JSON
        rental["usuario"] = a.usuario;            // olha só como é mais organizado usando JSON
        j["rentals"].push_back(rental);           // olha só como é mais organizado usando JSON
    }
    
    std::ofstream file(RENTALS_FILE);
    if (file.is_open()) {
        file << j.dump(2);
        file.close();
    }
}

void AlugarCarro(std::vector<Carro>& carros, const std::string& usuario) {
    std::cout << "=== Alugar Carro ===\n";
    
  
    std::vector<int> indicesDisponiveis;
    int idx = 0;
    for (const auto& c : carros) {
        if (c.disponivel) {
            std::cout << (indicesDisponiveis.size() + 1) << ". Modelo: " << c.modelo
                      << ", Ano: " << c.ano
                      << ", Preço/dia: R$ " << std::fixed << std::setprecision(2) << c.precoPorDia << "\n";
            indicesDisponiveis.push_back(idx);
        }
        ++idx;
    }
    
    if (indicesDisponiveis.empty()) {
        std::cout << "Nenhum carro disponível no momento.\n";
        return;
    }
    
    std::cout << "Escolha o número do carro: ";
    int escolha;
    if (!(std::cin >> escolha) || escolha < 1 || escolha > static_cast<int>(indicesDisponiveis.size())) {
        system("cls");
        std::cout << "Opção inválida.\n";
        return;
    }
    std::cin.ignore();
    
    int indiceCarro = indicesDisponiveis[escolha - 1];
    Carro& carroSelecionado = carros[indiceCarro];
    
    std::cout << "Data de início (YYYY-MM-DD): ";
    std::string dataInicio;
    std::getline(std::cin, dataInicio);
    
    if (dataInicio.empty()) {
        dataInicio = ObterDataAtual();
    }
    
    std::cout << "Data de fim (YYYY-MM-DD): ";
    std::string dataFim;
    std::getline(std::cin, dataFim);
    
    if (dataFim.empty()) {
        std::cout << "Data de fim é obrigatória.\n";
        return;
    }
    
    int dias = CalcularDias(dataInicio, dataFim);
   
    
    // Criar aluguel
    Aluguel novoAluguel;
    novoAluguel.modelo = carroSelecionado.modelo;
    novoAluguel.dataInicio = dataInicio;
    novoAluguel.dataFim = dataFim;
    novoAluguel.dias = dias;
    novoAluguel.precoPorDia = carroSelecionado.precoPorDia;
    novoAluguel.total = 0.0;  
    novoAluguel.devolvido = false;
    novoAluguel.usuario = usuario;
    
 
    auto alugueis = CarregarAlugueis();
    alugueis.push_back(novoAluguel);
    SalvarAlugueis(alugueis);
    
    // Marcar carro como indisponível
    carroSelecionado.disponivel = false;
    saveCarros(carros);
    
    std::cout << "Carro alugado com sucesso!\n";
    std::cout << "Período: " << dataInicio << " até " << dataFim << " (" << dias << " dias)\n";
}

void DevolverCarro(std::vector<Carro>& carros) {
    std::cout << "=== Devolver Carro ===\n";
    
    auto alugueis = CarregarAlugueis();
    std::vector<int> indicesAtivos;
    
    int idx = 0;
    for (const auto& a : alugueis) {
        if (!a.devolvido) {
            std::cout << (indicesAtivos.size() + 1) << ". Modelo: " << a.modelo
                      << ", Data início: " << a.dataInicio
                      << ", Data fim: " << a.dataFim
                      << ", Dias: " << a.dias << "\n";
            indicesAtivos.push_back(idx);
        }
        ++idx;
    }
    
    if (indicesAtivos.empty()) {
        std::cout << "Nenhum aluguel ativo encontrado.\n";
        return;
    }
    
    std::cout << "Escolha o número do aluguel para devolver: ";
    int escolha;
    if (!(std::cin >> escolha) || escolha < 1 || escolha > static_cast<int>(indicesAtivos.size())) {
        system("cls");
        std::cout << "Opção inválida.\n";
        std::cout << "Opção inválida.\n";
        return;
    }
    std::cin.ignore();
    
    int indiceAluguel = indicesAtivos[escolha - 1];
    Aluguel& aluguel = alugueis[indiceAluguel];
    
 
    aluguel.total = aluguel.dias * aluguel.precoPorDia;
    aluguel.devolvido = true;
 
    for (auto& c : carros) {
        if (c.modelo == aluguel.modelo) {
            c.disponivel = true;
            break;
        }
    }
    
    SalvarAlugueis(alugueis);
    saveCarros(carros);
    
    std::cout << "Carro devolvido com sucesso!\n";
    std::cout << "Total a pagar: R$ " << std::fixed << std::setprecision(2) << aluguel.total << "\n";
    std::cout << "Detalhes: " << aluguel.dias << " dias x R$ " << aluguel.precoPorDia << " = R$ " << aluguel.total << "\n";
}

void ExibirHistoricoAlugueis() {
    std::cout << "=== Histórico de Aluguéis ===\n";
    
    auto alugueis = CarregarAlugueis();
    
    if (alugueis.empty()) {
        std::cout << "Nenhum aluguel registrado.\n";
        return;
    }
    
    for (int i = 0; i < alugueis.size(); i++) {
        const auto& a = alugueis[i];
        printf("%d. Modelo: %s, Data Início: %s, Data Fim: %s, Dias: %d, Preço/Dia: R$ %.2f", 
               i + 1, a.modelo.c_str(), a.dataInicio.c_str(), a.dataFim.c_str(), a.dias, a.precoPorDia);
        
        if (a.devolvido) {
            printf(", Total: R$ %.2f, Status: Devolvido", a.total);
        } else {
            printf(", Total: Pendente, Status: Ativo");
        }
        
        printf(", Usuário: %s\n", a.usuario.c_str());
    }
}

std::string realizarLogin(const std::vector<Usuario>& users) {
    std::cout << "=== Login ===\n";
    for (int tent = 0; tent < 3; ++tent) {
        std::string u, p;
        std::cout << "Usuário: ";
        std::getline(std::cin, u);
        std::cout << "Senha: ";
        std::getline(std::cin, p);
        for (const auto& user : users) {
            if (user.username == u && user.password == p) {
                std::cout << "Login bem-sucedido!\n";
                return user.username;
            }
        }
        std::cout << "Credenciais inválidas. Tente novamente.\n";
    }
    std::cout << "Número máximo de tentativas atingido.\n";
    return "";
}

void cadastrarCarro(std::vector<Carro>& cars) {
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

void listarDisponiveis(const std::vector<Carro>& cars) {
    std::cout << "=== Carros Disponíveis ===\n";
    int count = 0;
    for (const auto& c : cars) {
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
    setlocale(LC_ALL, "pt_BR.UTF-8");
    ensureDataFiles();

    auto usuarios = loadUsuarios();
    auto carros = loadCarros();

    // Garantir que std::getline funcione após possíveis leituras anteriores
    std::cin.clear();

    std::string usuarioLogado = realizarLogin(usuarios);
    if (usuarioLogado.empty()) {
        return 0;
    }

    while (true) {
        std::cout << "\n=== Menu ===\n";
        std::cout << "1. Cadastrar Carro\n";
        std::cout << "2. Listar Carros Disponíveis\n";
        std::cout << "3. Alugar Carro\n";
        std::cout << "4. Devolver Carro\n";
        std::cout << "5. Exibir Histórico de Aluguéis\n";
        std::cout << "6. Sair\n";
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
        }
        else if (op == 2) {
            listarDisponiveis(carros);
        }
        else if (op == 3) {
            AlugarCarro(carros, usuarioLogado);
        }
        else if (op == 4) {
            DevolverCarro(carros);
        }
        else if (op == 5) {
            ExibirHistoricoAlugueis();
        }
        else if (op == 6) {
            std::cout << "Saindo...\n";
            break;
        }
        else {
            std::cout << "Opção não reconhecida.\n";
        }
    }

    return 0;
}
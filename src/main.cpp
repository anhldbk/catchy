#include <memory>
#include <stdio.h>
#include <iostream>

using namespace std;

class Ultron{
private:
    Ultron();
    static weak_ptr<Ultron> _instance;
public:
    static shared_ptr<Ultron> get_instance();
    void show(){
        cout << "Hi, I am an Ultron\n";
    }
    ~Ultron(){
        cout << "Destructed\n";
    }
};

weak_ptr<Ultron> Ultron::_instance(new Ultron());

int main(){
    cout << "Data\n";
    shared_ptr<Ultron> u_ptr = Ultron::get_instance();
    cout << u_ptr.use_count() << endl;
    u_ptr->show();    
}

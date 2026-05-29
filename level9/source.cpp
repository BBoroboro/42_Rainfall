#include <cstdlib>  // pour exit
#include <string>

class N {
public:
    N(int value) {
        // constructeur avec un int
        // implémentation originale inconnue
    }

    void setAnnotation(const char* annotation) {
        // implémentation originale inconnue
    }

    void callOther(N* other) {
        // correspond à (*(code *)**(undefined4 **)this_00)(this_00,this);
        // probablement un appel à une méthode virtuelle
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        exit(1);  // correspond à _exit(1)
    }

    N* n1 = new N(5);
    N* n2 = new N(6);

    n1->setAnnotation(argv[1]);  // argv[1] correspond à *(char **)(param_2 + 4) dans le décompilé

    n2->callOther(n1);

    // éventuellement libérer la mémoire
    delete n1;
    delete n2;

    return 0;
}
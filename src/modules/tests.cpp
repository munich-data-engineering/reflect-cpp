#include <string>
#include <vector>

import rfl;

using String = std::string;
template <typename T>
using Vector = std::vector<T>;

using rfl::Email;
using rfl::Minimum;
using rfl::Maximum;
using rfl::Rename;
using rfl::Timestamp;
using rfl::Validator;

using Age = Validator<unsigned int, Minimum<0>, Maximum<130>>;

struct Person {
    Rename<"firstName", String> first_name;
    Rename<"lastName", String> last_name = "Simpson";
    String town = "Springfield";
    Timestamp<"%Y-%m-%d"> birthday;
    Age age;
    Email email;
    Vector<Person> child;
};

void test_json() {
    const Person bart{
        .first_name = "Bart",
        .birthday = "1987-04-19",
        .age = 10,
        .email = "bart@simpson.com"
    };

    const Person lisa{
        .first_name = "Lisa",
        .birthday = "1987-04-19",
        .age = 8,
        .email = "lisa@simpson.com"
    };

    const Person maggie{
        .birthday = "1987-04-19",
        .age = 0,
        .email = "maggie@simpson.com"
    };

    const Person homer{
        .first_name = "Homer",
        .birthday = "1987-04-19",
        .age = 45,
        .email = "homer@simpson.com",
        .child = Vector<Person>({bart, lisa, maggie})
    };
}

int main() {
    test_json();
    return 0;
}

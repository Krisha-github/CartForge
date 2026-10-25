#include "cart.h"
#include "db_connection.h"
#include "utils.h"

#include <climits>
#include <iostream>
#include <string>

using namespace std;

Cart::Cart(MYSQL *connection)
{
    conn = connection;
}

void Cart::addToCart()
{
    int userId = util::readInt("Enter user ID: ", 1, INT_MAX);
    int productId = util::readInt("Enter product ID: ", 1, INT_MAX);
    int quantity = util::readInt("Enter quantity: ", 1, 100000);

    string uid = to_string(userId);
    string pid = to_string(productId);
    string qty = to_string(quantity);

    if (!rowExists(conn, "SELECT 1 FROM users WHERE user_id = ?", {uid}))
    {
        cout << "User " << userId << " does not exist.\n";
        return;
    }

    // Product must exist and be active; read its stock.
    Statement prod(conn, "SELECT stock_quantity FROM products WHERE product_id = ? AND is_active = 1");
    vector<string> row;
    if (!prod.execute({pid}) || !prod.fetch(row))
    {
        cout << "Product " << productId << " does not exist or is no longer available.\n";
        return;
    }
    long long stock = stoll(row[0]);

    // How many are already in this user's cart?
    long long inCart = 0;
    Statement existing(conn, "SELECT quantity FROM cart_items WHERE user_id = ? AND product_id = ?");
    if (existing.execute({uid, pid}) && existing.fetch(row))
        inCart = stoll(row[0]);

    if (inCart + quantity > stock)
    {
        cout << "Not enough stock: " << stock << " available, " << inCart
             << " already in your cart.\n";
        return;
    }

    Statement stmt(conn,
                   "INSERT INTO cart_items (user_id, product_id, quantity) VALUES (?, ?, ?) "
                   "ON DUPLICATE KEY UPDATE quantity = quantity + CAST(? AS SIGNED)");

    if (stmt.execute({uid, pid, qty, qty}))
        cout << "Item added to cart.\n";
    else
        cout << "Error: " << stmt.errorMessage() << endl;
}

void Cart::viewCart()
{
    int userId = util::readInt("Enter user ID: ", 1, INT_MAX);
    string uid = to_string(userId);

    Statement stmt(conn,
                   "SELECT c.cart_id, p.product_name, c.quantity, p.price, "
                   "(c.quantity * p.price) AS subtotal "
                   "FROM cart_items c "
                   "JOIN products p ON c.product_id = p.product_id "
                   "WHERE c.user_id = ? ORDER BY c.cart_id");

    if (!stmt.execute({uid}))
    {
        cout << "Error: " << stmt.errorMessage() << endl;
        return;
    }

    util::printTable({"Cart ID", "Product", "Qty", "Price", "Subtotal"}, stmt.fetchAll());

    Statement total(conn,
                    "SELECT IFNULL(SUM(c.quantity * p.price), 0) "
                    "FROM cart_items c JOIN products p ON c.product_id = p.product_id "
                    "WHERE c.user_id = ?");
    vector<string> row;
    if (total.execute({uid}) && total.fetch(row))
        cout << "\nCart total: " << row[0] << endl;
}

void Cart::removeFromCart()
{
    int userId = util::readInt("Enter user ID: ", 1, INT_MAX);
    int cartId = util::readInt("Enter cart item ID to remove: ", 1, INT_MAX);

    // Both IDs are required so a user can only remove items from their own cart.
    Statement stmt(conn, "DELETE FROM cart_items WHERE cart_id = ? AND user_id = ?");

    if (!stmt.execute({to_string(cartId), to_string(userId)}))
        cout << "Error: " << stmt.errorMessage() << endl;
    else if (stmt.affectedRows() == 0)
        cout << "No such item in that user's cart.\n";
    else
        cout << "Item removed from cart.\n";
}

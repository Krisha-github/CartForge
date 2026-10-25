#ifndef ORDER_H
#define ORDER_H

#include <mysql.h>

#include <string>

class OrderManager
{
private:
    MYSQL *conn;

    enum class Outcome
    {
        Success,
        Rejected, // business rule failed (empty cart, not enough stock, ...)
        Error,    // database error
        Deadlock  // MySQL picked this transaction as a deadlock victim: retry
    };

    struct Result
    {
        Outcome outcome = Outcome::Error;
        std::string message;
        std::string orderId;
        std::string total;
    };

    // One full checkout attempt inside a single transaction.
    Result attemptOrder(const std::string &userId);

public:
    explicit OrderManager(MYSQL *connection);

    void placeOrder();
    void viewOrderHistory();
    void lowStockReport();
    void totalSalesReport();
    void topSellingProducts();
};

#endif

using Microsoft.EntityFrameworkCore;

namespace Zpd.Api.Data;

public sealed class AppDbContext(DbContextOptions<AppDbContext> options) : DbContext(options)
{
}

using Microsoft.AspNetCore.Mvc;
using Zpd.Api.Data;

namespace Zpd.Api.Controllers;

[ApiController]
[Route("api/connection")]
public sealed class ConnectionController(AppDbContext db) : ControllerBase
{
    [HttpGet]
    [ProducesResponseType(StatusCodes.Status200OK)]
    [ProducesResponseType<ProblemDetails>(StatusCodes.Status503ServiceUnavailable)]
    public async Task<IActionResult> Get(CancellationToken cancellationToken)
    {
        if (!await db.Database.CanConnectAsync(cancellationToken))
        {
            return Problem(
                statusCode: StatusCodes.Status503ServiceUnavailable,
                title: "Database connection failed.");
        }

        return Ok(new { status = "ok", database = "connected" });
    }
}

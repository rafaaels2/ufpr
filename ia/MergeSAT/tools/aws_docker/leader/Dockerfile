################### Use MergeSat
FROM satcomp-mergesat:common AS builder
USER root


################### Extract MergeSat in run stage
FROM satcomp-infrastructure:leader AS mergesat_liaison
WORKDIR /

# Show all files we copied
RUN ls /competition

# Copy solver binary and solver scripts
COPY --from=builder --chown=ecs-user /mergesat/build/release/bin/mergesat /competition/mergesat
COPY --chown=ecs-user ./init_solver.sh /competition
COPY --chown=ecs-user ./run_solver.sh /competition
COPY --chown=ecs-user ./solver /competition

# Show all files we copied
RUN ls /competition

USER ecs-user
RUN chmod +x /competition/mergesat
RUN chmod +x /competition/init_solver.sh
RUN chmod +x /competition/run_solver.sh
RUN chmod +x /competition/solver
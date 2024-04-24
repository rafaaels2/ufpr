require 'rubygems'
require 'active_record'

ActiveRecord::Base.establish_connection :adapter => "sqlite3",
                                        :database => "Tabelas.sqlite3"

ActiveRecord::Base.connection.create_table :disciplinas_pessoas, id: false do |t|
    t.references :disciplina
    t.references :pessoa
end
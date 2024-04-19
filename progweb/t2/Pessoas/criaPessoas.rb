require 'rubygems'
require 'active_record'

ActiveRecord::Base.establish_connection :adapter => "sqlite3",
                                        :database => "Tabelas.sqlite3"

ActiveRecord::Base.connection.create_table :pessoas do |t|
    t.string :rg, limit: 4
    t.string :nome
    t.string :idade, limit: 3
    t.string :cidade
end